#pragma once

#include "app.hpp"
#include "websocket.hpp"

#include <boost/process/async_pipe.hpp>
#include <boost/process/child.hpp>
#include <boost/process/io.hpp>

#include <array>
#include <csignal>

namespace crow
{
namespace obmc_vm
{

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static crow::websocket::Connection* session = nullptr;

// The max network block device buffer size is 128kb plus 16bytes
// for the message header:
// https://github.com/NetworkBlockDevice/nbd/blob/master/doc/proto.md#simple-reply-message
static constexpr auto nbdBufferSize = (128 * 1024 + 16) * 4;

class Handler : public std::enable_shared_from_this<Handler>
{
  public:
    Handler(const std::string& mediaIn, boost::asio::io_context& ios) :
        pipeOut(ios), pipeIn(ios), media(mediaIn)
    {}

    ~Handler() = default;

    Handler(const Handler&) = delete;
    Handler(Handler&&) = delete;
    Handler& operator=(const Handler&) = delete;
    Handler& operator=(Handler&&) = delete;

    void doClose()
    {
        // boost::process::child::terminate uses SIGKILL, need to send SIGTERM
        // to allow the proxy to stop nbd-client and the USB device gadget.
        int rc = kill(proxy.id(), SIGTERM);
        if (rc != 0)
        {
            BMCWEB_LOG_ERROR("Failed to terminate nbd-proxy: {}", errno);
            return;
        }

        proxy.wait();
    }

    void connect()
    {
        std::error_code ec;
        proxy = boost::process::child("/usr/bin/nbd-proxy", media,
                                      boost::process::std_out > pipeOut,
                                      boost::process::std_in < pipeIn, ec);
        if (ec)
        {
            BMCWEB_LOG_ERROR("Couldn't connect to nbd-proxy: {}", ec.message());
            if (session != nullptr)
            {
                session->close("Error connecting to nbd-proxy");
            }
            return;
        }
        doRead();
    }

    void doWrite(std::string_view data)
    {
        pipeIn.async_write_some(
            boost::asio::buffer(data),
            [this, self(shared_from_this())](const boost::beast::error_code& ec,
                                             size_t bytesWritten) {
            BMCWEB_LOG_DEBUG("Wrote {} bytes", bytesWritten);

            if (session == nullptr)
            {
                return;
            }
            if (ec == boost::asio::error::eof)
            {
                session->close("VM socket port closed");
                return;
            }
            if (ec)
            {
                session->close("Error in writing to proxy port");
                BMCWEB_LOG_ERROR("Error in VM socket write {}", ec);
                return;
            }
            session->resumeRead();
        });
    }

    void doRead()
    {
        pipeOut.async_read_some(
            boost::asio::buffer(outputBuffer),
            [this, self(shared_from_this())](
                const boost::system::error_code& ec, std::size_t bytesRead) {
            BMCWEB_LOG_DEBUG("Read done.  Read {} bytes", bytesRead);
            if (ec)
            {
                BMCWEB_LOG_ERROR("Couldn't read from VM port: {}", ec);
                if (session != nullptr)
                {
                    session->close("Error in connecting to VM port");
                }
                return;
            }
            if (session == nullptr)
            {
                return;
            }

            std::string_view payload(outputBuffer.data(), bytesRead);
            session->sendEx(crow::websocket::MessageType::Binary, payload,
                            [self2{shared_from_this()}]() { self2->doRead(); });
        });
    }

    boost::process::async_pipe pipeOut;
    boost::process::async_pipe pipeIn;
    boost::process::child proxy;
    std::string media;
    bool doingWrite{false};

    std::array<char, 4096> outputBuffer{};
    std::array<char, nbdBufferSize> inputBuffer{};
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static std::shared_ptr<Handler> handler;

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/vm/0/0")
        .privileges({{"ConfigureComponents", "ConfigureManager"}})
        .websocket()
        .onopen([](crow::websocket::Connection& conn) {
        BMCWEB_LOG_DEBUG("Connection {} opened", logPtr(&conn));

        if (session != nullptr)
        {
            conn.close("Session already connected");
            return;
        }

        if (handler != nullptr)
        {
            conn.close("Handler already running");
            return;
        }

        session = &conn;

        // media is the last digit of the endpoint /vm/0/0. A future
        // enhancement can include supporting different endpoint values.
        const char* media = "0";
        handler = std::make_shared<Handler>(media, conn.getIoContext());
        handler->connect();
    })
        .onclose([](crow::websocket::Connection& conn,
                    const std::string& /*reason*/) {
        if (&conn != session)
        {
            return;
        }

        session = nullptr;
        handler->doClose();
        handler.reset();
    })
        .onmessage(
            [](crow::websocket::Connection& conn, std::string_view data, bool) {
        conn.deferRead();
        handler->doWrite(data);
    });
}

} // namespace obmc_vm
} // namespace crow
