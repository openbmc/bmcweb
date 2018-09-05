#pragma once
#include <array>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/beast/websocket.hpp>
#include <functional>

#include "crow/http_request.h"
#include "crow/socket_adaptors.h"

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
    explicit Connection(const crow::Request& req) :
        req(req), userdataPtr(nullptr){};

    virtual void sendBinary(const boost::beast::string_view msg) = 0;
    virtual void sendBinary(std::string&& msg) = 0;
    virtual void sendText(const boost::beast::string_view msg) = 0;
    virtual void sendText(std::string&& msg) = 0;
    virtual void close(const boost::beast::string_view msg = "quit") = 0;
    virtual boost::asio::io_service& getIoService() = 0;
    virtual ~Connection() = default;

    void userdata(void* u)
    {
        userdataPtr = u;
    }
    void* userdata()
    {
        return userdataPtr;
    }

    crow::Request req;

  private:
    void* userdataPtr;
};

template <typename Adaptor> class ConnectionImpl : public Connection
{
  public:
    ConnectionImpl(
        const crow::Request& req, Adaptor&& adaptorIn,
        std::function<void(Connection&)> open_handler,
        std::function<void(Connection&, const std::string&, bool)>
            message_handler,
        std::function<void(Connection&, const std::string&)> close_handler,
        std::function<void(Connection&)> error_handler) :
        adaptor(std::move(adaptorIn)),
        ws(adaptor.socket()), Connection(req),
        openHandler(std::move(open_handler)),
        messageHandler(std::move(message_handler)),
        closeHandler(std::move(close_handler)),
        errorHandler(std::move(error_handler))
    {
        BMCWEB_LOG_DEBUG << "Creating new connection " << this;
    }

    boost::asio::io_service& getIoService() override
    {
        return adaptor.getIoService();
    }

    void start()
    {
        BMCWEB_LOG_DEBUG << "starting connection " << this;

        boost::string_view protocol = req.getHeaderValue(
            boost::beast::http::field::sec_websocket_protocol);

        // Perform the websocket upgrade
        ws.async_accept_ex(
            req.req,
            [protocol{std::string(protocol)}](
                boost::beast::websocket::response_type& m) {
                if (!protocol.empty())
                {
                    m.insert(boost::beast::http::field::sec_websocket_protocol,
                             protocol);
                }
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

    void sendBinary(const boost::beast::string_view msg) override
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

    void sendText(const boost::beast::string_view msg) override
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

    void close(const boost::beast::string_view msg) override
    {
        ws.async_close(
            boost::beast::websocket::close_code::normal,
            [this, self(shared_from_this())](boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Error closing websocket " << ec;
                    return;
                }
                adaptor.close();
            });
    }

    void acceptDone()
    {
        BMCWEB_LOG_DEBUG << "Websocket accepted connection";

        if (openHandler)
        {
            openHandler(*this);
        }
        doRead();
    }

    void doRead()
    {
        ws.async_read(
            inBuffer, [this, self(shared_from_this())](
                          boost::beast::error_code ec, std::size_t bytes_read) {
                if (ec)
                {
                    if (ec != boost::beast::websocket::error::closed)
                    {
                        BMCWEB_LOG_ERROR << "doRead error " << ec;
                    }
                    if (closeHandler)
                    {
                        boost::beast::string_view reason = ws.reason().reason;
                        closeHandler(*this, std::string(reason));
                    }
                    return;
                }
                if (messageHandler)
                {
                    // TODO(Ed) There must be a more direct way to do this
                    // conversion, but I can't find it at the moment.  It should
                    // get optimized away
                    boost::asio::const_buffer cb =
                        boost::beast::buffers_front(inBuffer.data());
                    boost::beast::string_view message(
                        reinterpret_cast<char const*>(cb.data()), cb.size());
                    messageHandler(*this, std::string(message), ws.got_text());
                }
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
    Adaptor adaptor;

    boost::beast::websocket::stream<
        std::add_lvalue_reference_t<typename Adaptor::streamType>>
        ws;

    boost::beast::flat_static_buffer<4096> inBuffer;
    std::vector<std::string> outBuffer;
    bool doingWrite = false;

    std::function<void(Connection&)> openHandler;
    std::function<void(Connection&, const std::string&, bool)> messageHandler;
    std::function<void(Connection&, const std::string&)> closeHandler;
    std::function<void(Connection&)> errorHandler;
};
} // namespace websocket
} // namespace crow
