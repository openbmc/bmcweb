#pragma once
#include "http_request.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/beast/http/buffer_body.hpp>
#include <boost/beast/websocket.hpp>

#include <array>
#include <functional>

#ifdef BMCWEB_ENABLE_SSL
#include <boost/beast/websocket/ssl.hpp>
#endif

namespace crow
{
namespace sse
{

struct Connection : std::enable_shared_from_this<Connection>
{
  public:
    Connection(const crow::Request& reqIn, std::string user) :
        req(reqIn), userName{std::move(user)}
    {}
    virtual ~Connection() = default;

    virtual boost::asio::io_context& getIoContext() = 0;
    virtual void sendSSEHeader() = 0;
    virtual void completeRequest() = 0;
    virtual void close(const std::string_view msg = "quit") = 0;
    virtual void sendEvent(const std::string& id, const std::string& msg) = 0;

    const std::string& getUserName() const
    {
        return userName;
    }

    crow::Request req;
    crow::Response res;

  private:
    std::string userName{};
};

template <typename Adaptor>
class ConnectionImpl : public Connection
{
  public:
    ConnectionImpl(
        const crow::Request& reqIn, Adaptor adaptorIn,
        std::function<void(std::shared_ptr<Connection>&, const crow::Request&,
                           crow::Response&)>
            openHandler,
        std::function<void(std::shared_ptr<Connection>&)> closeHandler) :
        Connection(reqIn, reqIn.session->username),
        adaptor(std::move(adaptorIn)), openHandler(std::move(openHandler)),
        closeHandler(std::move(closeHandler))
    {
        BMCWEB_LOG_DEBUG << "ConnectionImpl: SSE constructor " << this;
    }

    ~ConnectionImpl() override
    {
        res.completeRequestHandler = nullptr;
        BMCWEB_LOG_DEBUG << "ConnectionImpl: SSE destructor" << this;
    }

    boost::asio::io_context& getIoContext() override
    {
        return static_cast<boost::asio::io_context&>(
            adaptor.get_executor().context());
    }

    void start()
    {
        // Register for completion callback.
        res.completeRequestHandler = [this, self(shared_from_this())] {
            boost::asio::post(this->adaptor.get_executor(),
                              [self] { self->completeRequest(); });
        };

        if (openHandler)
        {
            std::shared_ptr<Connection> self = this->shared_from_this();
            openHandler(self, req, res);
        }
    }

    void close(const std::string_view msg) override
    {
        BMCWEB_LOG_DEBUG << "Closing SSE connection " << this << " - " << msg;
        if constexpr (std::is_same_v<Adaptor,
                                     boost::beast::ssl_stream<
                                         boost::asio::ip::tcp::socket>>)
        {
            adaptor.next_layer().close();
        }
        else
        {
            adaptor.close();
        }

        // send notification to handler for cleanup
        if (closeHandler)
        {
            std::shared_ptr<Connection> self = this->shared_from_this();
            closeHandler(self);
        }
    }

    void sendSSEHeader() override
    {
        BMCWEB_LOG_DEBUG << "starting SSE connection ";
        using BodyType = boost::beast::http::buffer_body;
        auto response =
            std::make_shared<boost::beast::http::response<BodyType>>(
                boost::beast::http::status::ok, 11);
        auto serializer =
            std::make_shared<boost::beast::http::response_serializer<BodyType>>(
                *response);

        response->set(boost::beast::http::field::server, "iBMC");
        response->set(boost::beast::http::field::content_type,
                      "text/event-stream");
        response->body().data = nullptr;
        response->body().size = 0;
        response->body().more = true;

        boost::beast::http::async_write_header(
            adaptor, *serializer,
            [this, self(shared_from_this()), response, serializer](
                const boost::beast::error_code& ec, const std::size_t&) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Error sending header" << ec;
                    close("async_write_header failed");
                    return;
                }
                BMCWEB_LOG_DEBUG << "SSE header sent.";

                // SSE stream header sent, So lets setup monitor.
                // Any read data on this stream will be error in case of SSE.
                setupRead();
            });
    }

    void setupRead()
    {
        adaptor.async_read_some(
            outputBuffer.prepare(outputBuffer.capacity() - outputBuffer.size()),
            [this](const boost::system::error_code& ec, std::size_t bytesRead) {
                BMCWEB_LOG_DEBUG << "async_read_some: Read " << bytesRead
                                 << " bytes";
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Read error: " << ec;
                }
                outputBuffer.commit(bytesRead);
                outputBuffer.consume(bytesRead);

                // After establishing SSE connection, reading any data on this
                // connection is an error.
                close("Close SSE connection");
                return;
            });
    }

    void doWrite()
    {
        if (doingWrite)
        {
            return;
        }
        if (outBuffer.empty())
        {
            BMCWEB_LOG_DEBUG << "outBuffer is empty.. ";
            return;
        }
        doingWrite = true;

        adaptor.async_write_some(
            boost::asio::buffer(outBuffer.data(), outBuffer.size()),
            [this,
             self(shared_from_this())](boost::beast::error_code ec,
                                       const std::size_t& bytesTransferred) {
                doingWrite = false;
                outBuffer.erase(0, bytesTransferred);

                if (ec == boost::asio::error::eof)
                {
                    return;
                }

                if (ec)
                {
                    BMCWEB_LOG_ERROR << "async_write_some() failed: "
                                     << ec.message();
                    close("async_write_some failed");
                    return;
                }
                BMCWEB_LOG_DEBUG << "async_write_some() bytes transferred: "
                                 << bytesTransferred;

                doWrite();
            });
    }

    void completeRequest() override
    {
        BMCWEB_LOG_DEBUG << "SSE completeRequest() handler";
        if (res.body().empty() && !res.jsonValue.empty())
        {
            res.addHeader("Content-Type", "application/json");
            res.body() = res.jsonValue.dump(
                2, ' ', true, nlohmann::json::error_handler_t::replace);
        }

        res.preparePayload();
        auto serializer =
            std::make_shared<boost::beast::http::response_serializer<
                boost::beast::http::string_body>>(*res.stringResponse);

        boost::beast::http::async_write(
            adaptor, *serializer,
            [this, self(shared_from_this()),
             serializer](const boost::system::error_code& ec,
                         std::size_t bytesTransferred) {
                BMCWEB_LOG_DEBUG << this << " async_write " << bytesTransferred
                                 << " bytes";
                if (ec)
                {
                    BMCWEB_LOG_DEBUG << this << " from async_write failed";
                    return;
                }
                res.clear();

                BMCWEB_LOG_DEBUG << this
                                 << " Closing SSE connection - Request invalid";
                close("Request invalid");
            });

        // delete lambda with self shared_ptr
        // to enable connection destruction
        res.completeRequestHandler = nullptr;
    }

    void sendEvent(const std::string& id, const std::string& msg) override
    {
        if (msg.empty())
        {
            BMCWEB_LOG_DEBUG << "Empty data, bailing out.";
            return;
        }

        if (!id.empty())
        {
            outBuffer += "id: ";
            outBuffer.append(id.begin(), id.end());
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

  private:
    Adaptor adaptor;

    boost::beast::flat_static_buffer<1024U * 50U> outputBuffer;
    std::string outBuffer;
    bool doingWrite = false;

    std::function<void(std::shared_ptr<Connection>&, const crow::Request&,
                       crow::Response&)>
        openHandler;
    std::function<void(std::shared_ptr<Connection>&)> closeHandler;
};
} // namespace sse
} // namespace crow
