#pragma once
#include "async_resp.hpp"
#include "http_request.hpp"

#include <boost/asio/buffer.hpp>
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

struct Connection : std::enable_shared_from_this<Connection>
{
  public:
    explicit Connection(const crow::Request& reqIn) :
        req(reqIn.req), userdataPtr(nullptr)
    {}

    explicit Connection(const crow::Request& reqIn, std::string user) :
        req(reqIn.req), userName{std::move(user)}, userdataPtr(nullptr)
    {}

    Connection(const Connection&) = delete;
    Connection(Connection&&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection& operator=(const Connection&&) = delete;

    virtual void sendBinary(std::string_view msg) = 0;
    virtual void sendBinary(std::string&& msg) = 0;
    virtual void sendText(std::string_view msg) = 0;
    virtual void sendText(std::string&& msg) = 0;
    virtual void close(std::string_view msg = "quit") = 0;
    virtual boost::asio::io_context& getIoContext() = 0;
    virtual ~Connection() = default;

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
        std::function<void(Connection&)> openHandlerIn,
        std::function<void(Connection&, const std::string&, bool)>
            messageHandlerIn,
        std::function<void(Connection&, const std::string&)> closeHandlerIn,
        std::function<void(Connection&)> errorHandlerIn) :
        Connection(reqIn, reqIn.session == nullptr ? std::string{}
                                                   : reqIn.session->username),
        ws(std::move(adaptorIn)), inBuffer(inString, 131088),
        openHandler(std::move(openHandlerIn)),
        messageHandler(std::move(messageHandlerIn)),
        closeHandler(std::move(closeHandlerIn)),
        errorHandler(std::move(errorHandlerIn)), session(reqIn.session)
    {
        /* Turn on the timeouts on websocket stream to server role */
        ws.set_option(boost::beast::websocket::stream_base::timeout::suggested(
            boost::beast::role_type::server));
        BMCWEB_LOG_DEBUG << "Creating new connection " << this;
    }

    boost::asio::io_context& getIoContext() override
    {
        return static_cast<boost::asio::io_context&>(
            ws.get_executor().context());
    }

    void start()
    {
        BMCWEB_LOG_DEBUG << "starting connection " << this;

        using bf = boost::beast::http::field;

        std::string_view protocol = req[bf::sec_websocket_protocol];

        ws.set_option(boost::beast::websocket::stream_base::decorator(
            [session{session}, protocol{std::string(protocol)}](
                boost::beast::websocket::response_type& m) {

#ifndef BMCWEB_INSECURE_DISABLE_CSRF_PREVENTION
            if (session != nullptr)
            {
                // use protocol for csrf checking
                if (session->cookieAuth &&
                    !crow::utility::constantTimeStringCompare(
                        protocol, session->csrfToken))
                {
                    BMCWEB_LOG_ERROR << "Websocket CSRF error";
                    m.result(boost::beast::http::status::unauthorized);
                    return;
                }
            }
#endif
            if (!protocol.empty())
            {
                m.insert(bf::sec_websocket_protocol, protocol);
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

        // Perform the websocket upgrade
        ws.async_accept(req, [this, self(shared_from_this())](
                                 boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Error in ws.async_accept " << ec;
                return;
            }
            acceptDone();
        });
    }

    void sendBinary(const std::string_view msg) override
    {
        ws.binary(true);
        outBuffer.emplace_back(msg);
        doWrite();
    }

    void sendBinary(std::string&& msg) override
    {
        ws.binary(true);
        outBuffer.emplace_back(std::move(msg));
        doWrite();
    }

    void sendText(const std::string_view msg) override
    {
        ws.text(true);
        outBuffer.emplace_back(msg);
        doWrite();
    }

    void sendText(std::string&& msg) override
    {
        ws.text(true);
        outBuffer.emplace_back(std::move(msg));
        doWrite();
    }

    void close(const std::string_view msg) override
    {
        ws.async_close(
            {boost::beast::websocket::close_code::normal, msg},
            [self(shared_from_this())](boost::system::error_code ec) {
            if (ec == boost::asio::error::operation_aborted)
            {
                return;
            }
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Error closing websocket " << ec;
                return;
            }
            });
    }

    void acceptDone()
    {
        BMCWEB_LOG_DEBUG << "Websocket accepted connection";

        doRead();

        if (openHandler)
        {
            openHandler(*this);
        }
    }

    void doRead()
    {
        ws.async_read(inBuffer,
                      [this, self(shared_from_this())](
                          boost::beast::error_code ec, std::size_t bytesRead) {
            if (ec)
            {
                if (ec != boost::beast::websocket::error::closed)
                {
                    BMCWEB_LOG_ERROR << "doRead error " << ec;
                }
                if (closeHandler)
                {
                    std::string reason{ws.reason().reason.c_str()};
                    closeHandler(*this, reason);
                }
                return;
            }
            if (messageHandler)
            {
                messageHandler(*this, inString, ws.got_text());
            }
            inBuffer.consume(bytesRead);
            inString.clear();
            doRead();
        });
    }

    void doWrite()
    {
        // If we're already doing a write, ignore the request, it will be picked
        // up when the current write is complete
        if (doingWrite)
        {
            return;
        }

        if (outBuffer.empty())
        {
            // Done for now
            return;
        }
        doingWrite = true;
        ws.async_write(boost::asio::buffer(outBuffer.front()),
                       [this, self(shared_from_this())](
                           boost::beast::error_code ec, std::size_t) {
            doingWrite = false;
            outBuffer.erase(outBuffer.begin());
            if (ec == boost::beast::websocket::error::closed)
            {
                // Do nothing here.  doRead handler will call the
                // closeHandler.
                close("Write error");
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

  private:
    boost::beast::websocket::stream<Adaptor, false> ws;

    std::string inString;
    boost::asio::dynamic_string_buffer<std::string::value_type,
                                       std::string::traits_type,
                                       std::string::allocator_type>
        inBuffer;
    std::vector<std::string> outBuffer;
    bool doingWrite = false;

    std::function<void(Connection&)> openHandler;
    std::function<void(Connection&, const std::string&, bool)> messageHandler;
    std::function<void(Connection&, const std::string&)> closeHandler;
    std::function<void(Connection&)> errorHandler;
    std::shared_ptr<persistent_data::UserSession> session;
};
} // namespace websocket
} // namespace crow
