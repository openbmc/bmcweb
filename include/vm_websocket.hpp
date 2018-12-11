#pragma once
#include <crow/app.h>
#include <crow/websocket.h>

#include <boost/process.hpp>
#include <webserver_common.hpp>

namespace crow
{
namespace obmc_vm
{

static crow::websocket::Connection* session = nullptr;

static std::string inputBuffer;

// Match the buffer size of
// https://github.com/openbmc/jsnbd/blob/master/nbd-proxy.c
static std::array<char, 131072> outputBuffer;

static bool doingWrite = false;

class Handler
{
  public:
    Handler(boost::asio::io_service& ios) : ios(ios), pipeOut(ios), pipeIn(ios)
    {
        proxy = boost::process::child("/usr/local/sbin/nbd-proxy", "0",
                                      boost::process::std_out > pipeOut,
                                      boost::process::std_in < pipeIn);
        doWrite();
        doRead();
    }

    void doWrite()
    {
        if (doingWrite)
        {
            BMCWEB_LOG_DEBUG << "Already writing.  Bailing out";
            return;
        }

        if (inputBuffer.empty())
        {
            BMCWEB_LOG_DEBUG << "inputBuffer empty.  Bailing out";
            return;
        }

        pipeIn.async_write_some(
            boost::asio::buffer(inputBuffer.data(), inputBuffer.size()),
            [this](boost::beast::error_code ec, std::size_t bytes_written) {
                doingWrite = false;
                inputBuffer.erase(0, bytes_written);

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
                    BMCWEB_LOG_ERROR << "Error in VM pipe write " << ec;
                    return;
                }
                doWrite();
            });
    }

    void doRead()
    {
        pipeOut.async_read_some(
            boost::asio::buffer(outputBuffer.data(), outputBuffer.size()),
            [this](const boost::system::error_code& ec, std::size_t bytesRead) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Couldn't read from VM port: " << ec;
                    return;
                }
                if (session == nullptr)
                {
                    return;
                }

                boost::beast::string_view payload(outputBuffer.data(),
                                                  bytesRead);
                session->sendBinary(payload);
                doRead();
            });
    }

    boost::asio::io_service& ios;
    boost::process::child proxy;
    boost::process::async_pipe pipeOut;
    boost::process::async_pipe pipeIn;
};

std::unique_ptr<Handler> handler;

template <typename... Middlewares> void requestRoutes(Crow<Middlewares...>& app)
{
    // TODO Support /vm/0/<media> as an argument
    BMCWEB_ROUTE(app, "/vm/0/0")
        .websocket()
        .onopen([](crow::websocket::Connection& conn) {
            BMCWEB_LOG_DEBUG << "Connection " << &conn << " opened";

            if (session != nullptr)
            {
                conn.close("Session already connected");
                return;
            }

            session = &conn;

            if (handler == nullptr)
            {
                handler = std::make_unique<Handler>(conn.getIoService());
            }
        })
        .onclose(
            [](crow::websocket::Connection& conn, const std::string& reason) {
                session = nullptr;
                handler = nullptr;
                inputBuffer.clear();
                inputBuffer.shrink_to_fit();
            })
        .onmessage([](crow::websocket::Connection& conn,
                      const std::string& data, bool is_binary) {
            inputBuffer += data;
            handler->doWrite();
        });
}

} // namespace obmc_vm
} // namespace crow
