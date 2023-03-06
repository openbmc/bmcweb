#pragma once
#include "async_resp.hpp"
#include "http_request.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/websocket.hpp>

#include <array>
#include <functional>

#ifdef BMCWEB_ENABLE_SSL
#include <boost/beast/websocket/ssl.hpp>
#endif

namespace crow
{
namespace websocket
{

enum class MessageType
{
    Binary,
    Text,
};

struct Connection : std::enable_shared_from_this<Connection>
{
  public:
    Connection() = default;

    Connection(const Connection&) = delete;
    Connection(Connection&&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection& operator=(const Connection&&) = delete;

    virtual void sendEx(MessageType type, std::string_view msg,
                        std::function<void()>&& onDone) = 0;

    virtual void close(std::string_view msg = "quit") = 0;
    virtual void deferRead() = 0;
    virtual void resumeRead() = 0;
    virtual boost::asio::io_context& getIoContext() = 0;
    virtual ~Connection() = default;
    virtual boost::urls::url_view url() = 0;
};

template <typename Adaptor>
class ConnectionImpl : public Connection
{
    using self_t = ConnectionImpl<Adaptor>;

  public:
    ConnectionImpl(
        const boost::urls::url_view& urlViewIn,
        const std::shared_ptr<persistent_data::UserSession>& sessionIn,
        Adaptor adaptorIn, std::function<void(Connection&)> openHandlerIn,
        std::function<void(Connection&, std::string_view, bool)>
            messageHandlerIn,
        std::function<void(crow::websocket::Connection&, std::string_view,
                           crow::websocket::MessageType type, bool,
                           std::function<void()>&& whenComplete)>
            messageExHandlerIn,
        std::function<void(Connection&, const std::string&)> closeHandlerIn,
        std::function<void(Connection&)> errorHandlerIn) :
        uri(urlViewIn),
        ws(std::move(adaptorIn)), openHandler(std::move(openHandlerIn)),
        messageHandler(std::move(messageHandlerIn)),
        messageExHandler(std::move(messageExHandlerIn)),
        closeHandler(std::move(closeHandlerIn)),
        errorHandler(std::move(errorHandlerIn)), session(sessionIn)
    {
        /* Turn on the timeouts on websocket stream to server role */
        ws.set_option(boost::beast::websocket::stream_base::timeout::suggested(
            boost::beast::role_type::server));
        BMCWEB_LOG_DEBUG("Creating new connection {}", logPtr(this));
    }

    boost::asio::io_context& getIoContext() override
    {
        return static_cast<boost::asio::io_context&>(
            ws.get_executor().context());
    }

    void start(const crow::Request& req)
    {
        BMCWEB_LOG_DEBUG("starting connection {}", logPtr(this));

        using bf = boost::beast::http::field;
        std::string protocolHeader = req.req[bf::sec_websocket_protocol];

        ws.set_option(boost::beast::websocket::stream_base::decorator(
            [session{session},
             protocolHeader](boost::beast::websocket::response_type& m) {

#ifndef BMCWEB_INSECURE_DISABLE_CSRF_PREVENTION
            if (session != nullptr)
            {
                // use protocol for csrf checking
                if (session->cookieAuth &&
                    !crow::utility::constantTimeStringCompare(
                        protocolHeader, session->csrfToken))
                {
                    BMCWEB_LOG_ERROR("Websocket CSRF error");
                    m.result(boost::beast::http::status::unauthorized);
                    return;
                }
            }
#endif
            if (!protocolHeader.empty())
            {
                m.insert(bf::sec_websocket_protocol, protocolHeader);
            }

            m.insert(bf::strict_transport_security, "max-age=31536000; "
                                                    "includeSubdomains; "
                                                    "preload");
            m.insert(bf::pragma, "no-cache");
            m.insert(bf::cache_control, "no-Store,no-Cache");
            m.insert("Content-Security-Policy", "default-src 'self'");
            m.insert("X-XSS-Protection", "1; "
                                         "mode=block");
            m.insert("X-Content-Type-Options", "nosniff");
        }));

        // Make a pointer to keep the req alive while we accept it.
        using Body = boost::beast::http::request<bmcweb::HttpBody>;
        std::unique_ptr<Body> mobile = std::make_unique<Body>(req.req);
        Body* ptr = mobile.get();
        // Perform the websocket upgrade
        ws.async_accept(*ptr,
                        std::bind_front(&self_t::acceptDone, this,
                                        shared_from_this(), std::move(mobile)));
    }

    void sendEx(MessageType type, std::string_view msg,
                std::function<void()>&& onDone) override
    {
        if (doingWrite)
        {
            BMCWEB_LOG_CRITICAL(
                "Cannot mix sendEx usage with sendBinary or sendText");
            onDone();
            return;
        }
        ws.binary(type == MessageType::Binary);

        ws.async_write(boost::asio::buffer(msg),
                       [weak(weak_from_this()), onDone{std::move(onDone)}](
                           const boost::beast::error_code& ec, size_t) {
            std::shared_ptr<Connection> self = weak.lock();
            if (!self)
            {
                BMCWEB_LOG_ERROR("Connection went away");
                return;
            }

            // Call the done handler regardless of whether we
            // errored, but before we close things out
            onDone();

            if (ec)
            {
                BMCWEB_LOG_ERROR("Error in ws.async_write {}", ec);
                self->close("write error");
            }
        });
    }

    void close(std::string_view msg) override
    {
        ws.async_close(
            {boost::beast::websocket::close_code::normal, msg},
            [self(shared_from_this())](const boost::system::error_code& ec) {
            if (ec == boost::asio::error::operation_aborted)
            {
                return;
            }
            if (ec)
            {
                BMCWEB_LOG_ERROR("Error closing websocket {}", ec);
                return;
            }
        });
    }

    boost::urls::url_view url() override
    {
        return uri;
    }

    void acceptDone(const std::shared_ptr<Connection>& /*self*/,
                    const std::unique_ptr<
                        boost::beast::http::request<bmcweb::HttpBody>>& /*req*/,
                    const boost::system::error_code& ec)
    {
        if (ec)
        {
            BMCWEB_LOG_ERROR("Error in ws.async_accept {}", ec);
            return;
        }
        BMCWEB_LOG_DEBUG("Websocket accepted connection");

        if (openHandler)
        {
            openHandler(*this);
        }
        doRead();
    }

    void deferRead() override
    {
        readingDefered = true;

        // If we're not actively reading, we need to take ownership of
        // ourselves for a small portion of time, do that, and clear when we
        // resume.
        selfOwned = shared_from_this();
    }

    void resumeRead() override
    {
        readingDefered = false;
        doRead();

        // No longer need to keep ourselves alive now that read is active.
        selfOwned.reset();
    }

    void doRead()
    {
        if (readingDefered)
        {
            return;
        }
        ws.async_read_some(
            boost::asio::buffer(inBuffer),
            [this, self(shared_from_this())](const boost::beast::error_code& ec,
                                             size_t bytesRead) {
            handleMessage(self, ec, bytesRead);
        });
        // std::bind_front(&self_t::handleMessage, this, shared_from_this()));
    }

    void doWrite()
    {
        // If we're already doing a write, ignore the request, it will be picked
        // up when the current write is complete
        if (doingWrite)
        {
            return;
        }

        if (outBuffer.size() == 0)
        {
            // Done for now
            return;
        }
        doingWrite = true;
        ws.async_write(outBuffer.data(), [this, self(shared_from_this())](
                                             const boost::beast::error_code& ec,
                                             size_t bytesSent) {
            doingWrite = false;
            outBuffer.consume(bytesSent);
            if (ec == boost::beast::websocket::error::closed)
            {
                // Do nothing here.  doRead handler will call the
                // closeHandler.
                close("Write error");
                return;
            }
            if (ec)
            {
                BMCWEB_LOG_ERROR("Error in ws.async_write {}", ec);
                return;
            }
            doWrite();
        });
    }

  private:
    void handleMessage(const std::shared_ptr<Connection>& /*self*/,
                       const boost::beast::error_code& ec, size_t bytesRead)
    {
        if (ec)
        {
            if (ec != boost::beast::websocket::error::closed)
            {
                BMCWEB_LOG_ERROR("doRead error {}", ec);
            }
            if (closeHandler)
            {
                std::string reason{ws.reason().reason.data(),
                                   ws.reason().reason.size()};
                closeHandler(*this, reason);
            }
            return;
        }
        std::string_view inString(inBuffer.data(), bytesRead);
        if (messageExHandler)
        {
            // Note, because of the interactions with the read buffers,
            // this message handler overrides the normal message handler
            messageExHandler(*this, inString, MessageType::Binary,
                             ws.is_message_done(),
                             [this, self(shared_from_this())]() {
                if (self == nullptr)
                {
                    return;
                }

                doRead();
            });
            return;
        }

        if (messageHandler)
        {
            messageHandler(*this, inString, ws.got_text());
        }
        doRead();
    }

    boost::urls::url uri;

    boost::beast::websocket::stream<Adaptor, false> ws;

    bool readingDefered = false;
    std::array<char, 8196> inBuffer{};

    boost::beast::multi_buffer outBuffer;
    bool doingWrite = false;

    std::function<void(Connection&)> openHandler;
    std::function<void(Connection&, std::string_view, bool)> messageHandler;
    std::function<void(crow::websocket::Connection&, std::string_view,
                       crow::websocket::MessageType type, bool,
                       std::function<void()>&& whenComplete)>
        messageExHandler;
    std::function<void(Connection&, const std::string&)> closeHandler;
    std::function<void(Connection&)> errorHandler;
    std::shared_ptr<persistent_data::UserSession> session;

    std::shared_ptr<Connection> selfOwned;
};
} // namespace websocket
} // namespace crow
