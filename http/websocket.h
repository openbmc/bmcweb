#pragma once
#include <array>
#include <async_resp.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/beast/websocket.hpp>
#include <functional>

#include "http_request.h"

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
        req(reqIn.req), userdataPtr(nullptr){};

    virtual void sendBinary(const std::string_view msg) = 0;
    virtual void sendBinary(std::string&& msg) = 0;
    virtual void sendText(const std::string_view msg) = 0;
    virtual void sendText(std::string&& msg) = 0;
    virtual void close(const std::string_view msg = "quit") = 0;
    virtual boost::asio::io_context& get_io_context() = 0;
    virtual ~Connection() = default;

    void userdata(void* u)
    {
        userdataPtr = u;
    }
    void* userdata()
    {
        return userdataPtr;
    }

    boost::beast::http::request<boost::beast::http::string_body> req;
    crow::Response res;

  private:
    void* userdataPtr;
};

template <typename Adaptor> class ConnectionImpl : public Connection
{
  public:
    ConnectionImpl(
        const crow::Request& reqIn, Adaptor adaptorIn,
        std::function<void(Connection&, std::shared_ptr<bmcweb::AsyncResp>)>
            open_handler,
        std::function<void(Connection&, const std::string&, bool)>
            message_handler,
        std::function<void(Connection&, const std::string&)> close_handler,
        std::function<void(Connection&)> error_handler) :
        Connection(reqIn),
        ws(std::move(adaptorIn)), inString(), inBuffer(inString, 131088),
        openHandler(std::move(open_handler)),
        messageHandler(std::move(message_handler)),
        closeHandler(std::move(close_handler)),
        errorHandler(std::move(error_handler))
    {
        BMCWEB_LOG_DEBUG << "Creating new connection " << this;
    }

    boost::asio::io_context& get_io_context() override
    {
        return static_cast<boost::asio::io_context&>(
            ws.get_executor().context());
    }

    void start()
    {
        BMCWEB_LOG_DEBUG << "starting connection " << this;

        using bf = boost::beast::http::field;

        std::string_view protocol = req[bf::sec_websocket_protocol];

        // Perform the websocket upgrade
        ws.async_accept_ex(
            req,
            [protocol{std::string(protocol)}](
                boost::beast::websocket::response_type& m) {
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
            },
            [this, self(shared_from_this())](boost::system::error_code ec) {
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
            boost::beast::websocket::close_code::normal,
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

        auto asyncResp = std::make_shared<bmcweb::AsyncResp>(
            res, [this, self(shared_from_this())]() { doRead(); });

        asyncResp->res.result(boost::beast::http::status::ok);

        if (openHandler)
        {
            openHandler(*this, asyncResp);
        }
    }

    void doRead()
    {
        ws.async_read(inBuffer,
                      [this, self(shared_from_this())](
                          boost::beast::error_code ec, std::size_t bytes_read) {
                          if (ec)
                          {
                              if (ec != boost::beast::websocket::error::closed)
                              {
                                  BMCWEB_LOG_ERROR << "doRead error " << ec;
                              }
                              if (closeHandler)
                              {
                                  std::string_view reason = ws.reason().reason;
                                  closeHandler(*this, std::string(reason));
                              }
                              return;
                          }
                          if (messageHandler)
                          {
                              messageHandler(*this, inString, ws.got_text());
                          }
                          inBuffer.consume(bytes_read);
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
        ws.async_write(
            boost::asio::buffer(outBuffer.front()),
            [this, self(shared_from_this())](boost::beast::error_code ec,
                                             std::size_t bytes_written) {
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
    boost::beast::websocket::stream<Adaptor> ws;

    std::string inString;
    boost::asio::dynamic_string_buffer<std::string::value_type,
                                       std::string::traits_type,
                                       std::string::allocator_type>
        inBuffer;
    std::vector<std::string> outBuffer;
    bool doingWrite = false;

    std::function<void(Connection&, std::shared_ptr<bmcweb::AsyncResp>)>
        openHandler;
    std::function<void(Connection&, const std::string&, bool)> messageHandler;
    std::function<void(Connection&, const std::string&)> closeHandler;
    std::function<void(Connection&)> errorHandler;
};
} // namespace websocket
} // namespace crow
