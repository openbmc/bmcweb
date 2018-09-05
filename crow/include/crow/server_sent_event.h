#pragma once
#include <array>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/beast/websocket.hpp>
#include <functional>

#include "crow/http_request.h"

#ifdef BMCWEB_ENABLE_SSL
#include <boost/beast/websocket/ssl.hpp>
#endif

namespace crow
{

struct SseConnection : std::enable_shared_from_this<SseConnection>
{
  public:
    SseConnection(){};
    virtual void sendEvent(const boost::beast::string_view msg,
                           const boost::beast::string_view eventType = "",
                           const boost::beast::string_view id = "",
                           std::size_t retryMilliseconds = 0) = 0;
    virtual void sendComment(const boost::beast::string_view msg) = 0;
    virtual void close() = 0;
    virtual boost::asio::io_context& getIoService() = 0;

    virtual ~SseConnection() = default;

    virtual void userdata(void* u) = 0;
    virtual void* userdata() = 0;
};

template <typename Adaptor, typename OpenHandler, typename CloseHandler>
class SseConnectionImpl final : public SseConnection
{
  public:
    SseConnectionImpl(const crow::Request& req, Adaptor&& adaptorIn,
                      OpenHandler&& open_handler,
                      CloseHandler&& close_handler) :
        adaptor(std::move(adaptorIn)),
        timer(adaptor.getIoService()), SseConnection(),
        openHandler(std::move(open_handler)),
        closeHandler(std::move(close_handler)), req(req)
    {
        BMCWEB_LOG_DEBUG << "Creating new connection " << this;
        outBuffer.reserve(1024);
    }

    ~SseConnectionImpl()
    {
        BMCWEB_LOG_DEBUG << "Connection destroyed " << this;
    }

    boost::asio::io_context& getIoService() override
    {
        return adaptor.getIoService();
    }

    void start()
    {
        BMCWEB_LOG_DEBUG << "starting connection " << this;

        using bodyType = boost::beast::http::buffer_body;
        // Put the response and serializer on heap so they can survive while the
        // header is being sent.  They could be put into this class, but then we
        // would need to keep them in memory forever.
        auto response =
            std::make_shared<boost::beast::http::response<bodyType>>(
                boost::beast::http::status::ok, 11);
        auto serializer =
            std::make_shared<boost::beast::http::response_serializer<bodyType>>(
                *response);

        response->set(boost::beast::http::field::server, "iBMC");
        response->set(boost::beast::http::field::content_type,
                      "text/event-stream");
        response->body().data = nullptr;
        response->body().size = 0;
        response->body().more = true;
        doingWrite = true;

        boost::beast::http::async_write_header(
            adaptor.socket(), *serializer,
            [this, self(shared_from_this()), response, serializer](
                const boost::beast::error_code& ec, std::size_t bytes_written) {
                doingWrite = false;
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Error sending header" << ec;
                    return;
                }

                BMCWEB_LOG_DEBUG << "Header sent";

                openHandler(*this);

                doWrite();
            });

        adaptor.socket().async_read_some(
            boost::asio::buffer(inBuffer.data(), inBuffer.size()),
            [this, self(shared_from_this())](const boost::beast::error_code& ec,
                                             std::size_t bytesRead) {
                if (ec && ec != boost::asio::error::eof)
                {
                    BMCWEB_LOG_CRITICAL << "sse read error " << ec;
                }

                // ignore error code.  If we read any more bytes from the stream
                // after we've begun the sse, it's an error
                BMCWEB_LOG_CRITICAL << "Wait read read extra bytes";
                close();
            });

        timer.expires_after(std::chrono::seconds(15));
        timer.async_wait(
            [this, self(shared_from_this())](
                const boost::system::error_code& ec) { onTimerEvent(ec); });
    }

    void onTimerEvent(const boost::system::error_code& ec)
    {
        BMCWEB_LOG_DEBUG << "onTimerExpire Timer expired";
        if (ec == boost::asio::error::operation_aborted)
        {
            return;
        }
        if (ec)
        {
            BMCWEB_LOG_ERROR << "Timer failed " << ec;
            return;
        }

        sendComment("Keep alive");

        timer.expires_after(std::chrono::seconds(15));
        timer.async_wait(
            [this, self(shared_from_this())](
                const boost::system::error_code& ec) { onTimerEvent(ec); });
    }

    void sendEvent(const boost::beast::string_view msg,
                   const boost::beast::string_view eventType = "",
                   const boost::beast::string_view id = "",
                   std::size_t retryMilliseconds = 0) override
    {
        if (!eventType.empty())
        {
            outBuffer += "event: ";
            outBuffer.append(eventType.data(), eventType.size());
            outBuffer += "\n";
        }

        if (!id.empty())
        {
            outBuffer += "id: ";
            outBuffer.append(id.begin(), id.end());
            outBuffer += "\n";
        }

        if (retryMilliseconds != 0)
        {
            outBuffer += "retry: ";
            outBuffer += std::to_string(retryMilliseconds);
            outBuffer += "\n";
        }

        outBuffer += "data: ";
        for (char character : msg)
        {
            outBuffer += character;
            if (character == '\n')
            {
                outBuffer += "data: ";
            }
        }
        outBuffer += "\n\n";

        doWrite();
    }

    void sendComment(const boost::beast::string_view msg) override
    {
        outBuffer += ": ";
        outBuffer.append(msg.data(), msg.size());
        outBuffer += "\n\n";
        doWrite();
    }

    void close() override
    {
        timer.cancel();
        adaptor.close();

        closeHandler(*this);
    }

    void doWrite()
    {
        BMCWEB_LOG_DEBUG << "Writing: " << outBuffer;
        // If we're already doing a write, ignore the request, it will be picked
        // up when the current write is complete
        if (doingWrite)
        {
            BMCWEB_LOG_DEBUG << "Already writing.  Bailing out";
            return;
        }

        if (outBuffer.empty())
        {
            BMCWEB_LOG_DEBUG << "Outbuffer empty.  Bailing out";
            return;
        }
        doingWrite = true;

        adaptor.socket().async_write_some(
            boost::asio::buffer(outBuffer.data(), outBuffer.size()),
            [this, self(shared_from_this())](boost::beast::error_code ec,
                                             std::size_t bytes_written) {
                doingWrite = false;
                outBuffer.erase(0, bytes_written);

                if (ec == boost::asio::error::eof)
                {
                    return;
                }
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Error in ws.async_write " << ec;
                    return;
                }
                doWrite();
            });
    }

    void userdata(void* u)
    {
        userdataPtr = u;
    }
    void* userdata()
    {
        return userdataPtr;
    }

  private:
    Adaptor adaptor;

    boost::asio::steady_timer timer;

    std::string outBuffer;
    bool doingWrite = true;

    OpenHandler openHandler;
    CloseHandler closeHandler;

    void* userdataPtr = nullptr;

    crow::Request req;

    std::array<char, 1> inBuffer;
};
} // namespace crow
