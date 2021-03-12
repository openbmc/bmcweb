#pragma once
#include "http_request.hpp"

#include <async_resp.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/buffer.hpp>
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
        req(reqIn.req), userName{std::move(user)}, userdataPtr(nullptr)
    {
        BMCWEB_LOG_DEBUG << "SSE connection class construction";
    }

    virtual void sendEvent(const std::string& msg) = 0;
    virtual void close(const std::string_view msg = "quit") = 0;
    virtual boost::asio::io_context& getIoContext() = 0;
    // virtual ~Connection() = default;
    virtual ~Connection()
    {
        BMCWEB_LOG_DEBUG << "SSE connection class destructor... "
                            "Grrrrrrrrrrrrrrrrrrrrrrrrrrrrrr";
    }

    void userdata(void* u)
    {
        userdataPtr = u;
    }
    void* userdata()
    {
        return userdataPtr;
    }

    const std::string& getUserName() const
    {
        return userName;
    }

    boost::beast::http::request<boost::beast::http::string_body> req;
    crow::Response res;

  private:
    std::string userName{};
    void* userdataPtr;
};

template <typename Adaptor>
class ConnectionImpl : public Connection
{
  public:
    ConnectionImpl(
        const crow::Request& reqIn, Adaptor adaptorIn,
        std::function<void(std::shared_ptr<Connection>&,
                           std::shared_ptr<bmcweb::AsyncResp>)>
            openHandler,
        std::function<void(std::shared_ptr<Connection>&)> closeHandler) :
        Connection(reqIn, reqIn.session->username),
        adaptor(std::move(adaptorIn)), timer(getIoContext()),
        openHandler(std::move(openHandler)),
        closeHandler(std::move(closeHandler)), session(reqIn.session)
    {
        BMCWEB_LOG_DEBUG << "ConnectionImpl: constructor " << this;
    }
    virtual ~ConnectionImpl()
    {
        BMCWEB_LOG_DEBUG << "ConnectionImpl: destructor... "
                            "Grrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr ";
    }
    boost::asio::io_context& getIoContext() override
    {
        BMCWEB_LOG_DEBUG << "ConnectionImpl: getIoContext() called";
        return static_cast<boost::asio::io_context&>(
            adaptor.get_executor().context());
    }

    void start()
    {
        auto asyncResp = std::make_shared<bmcweb::AsyncResp>(res);
        if (openHandler)
        {
            std::shared_ptr<Connection> self = this->shared_from_this();
            openHandler(self, asyncResp);
        }

        // onTimerEvent();
        sendSSEHeader();
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

    void sendSSEHeader()
    {
        BMCWEB_LOG_DEBUG << "starting SSE connection ";
        using BodyType = boost::beast::http::buffer_body;
        auto response =
            std::make_shared<boost::beast::http::response<BodyType>>(
                boost::beast::http::status::ok, 11);
        auto serializer =
            std::make_shared<boost::beast::http::response_serializer<BodyType>>(
                *response);

        // TODO: Add hostname in http header.
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
                setupRead();
            });
    }

    void onTimerEvent()
    {
        BMCWEB_LOG_DEBUG << "onTimerExpire Timer expired";
        timer.expires_after(std::chrono::seconds(15));
        timer.async_wait([this, self(shared_from_this())](
                             const boost::system::error_code& ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Timer failed: " << ec;
                return;
            }
            onTimerEvent();
        });
    }

    void sendEvent(const std::string& msg) override
    {
        outBuffer = msg;
        outBuffer += "\n\n";
        doWrite();
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

  private:
    Adaptor adaptor;
    boost::asio::steady_timer timer;

    boost::beast::flat_static_buffer<1024U * 50U> outputBuffer;
    std::string outBuffer;
    bool doingWrite = false;

    std::function<void(std::shared_ptr<Connection>&,
                       std::shared_ptr<bmcweb::AsyncResp>)>
        openHandler;
    std::function<void(std::shared_ptr<Connection>&)> closeHandler;
    std::shared_ptr<persistent_data::UserSession> session;
};
} // namespace sse
} // namespace crow
