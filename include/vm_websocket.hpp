#pragma once

#include <crow/app.h>
#include <crow/websocket.h>
#include <signal.h>

#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/process.hpp>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <webserver_common.hpp>

namespace crow
{
namespace obmc_vm
{

static crow::websocket::Connection* session = nullptr;

// The max network block device buffer size is 128kb plus 16bytes
// for the message header:
// https://github.com/NetworkBlockDevice/nbd/blob/master/doc/proto.md#simple-reply-message
boost::beast::flat_static_buffer<131088> outputBuffer;
boost::beast::flat_static_buffer<131088> inputBuffer;

static bool doingWrite = false;

class Handler : public std::enable_shared_from_this<Handler>
{
  public:
    Handler(const std::string& media, boost::asio::io_service& ios) :
        pipeOut(ios), pipeIn(ios), media(media)
    {
    }

    ~Handler()
    {
    }

    void doClose()
    {
        // boost::process::child::terminate uses SIGKILL, need to send SIGTERM
        // to allow the proxy to stop nbd-client and the USB device gadget.
        int rc = kill(proxy.id(), SIGTERM);
        if (rc)
        {
            return;
        }
        proxy.wait();
    }

    void connect()
    {
        std::error_code ec;
        proxy = boost::process::child("/usr/sbin/nbd-proxy", media,
                                      boost::process::std_out > pipeOut,
                                      boost::process::std_in < pipeIn, ec);
        if (ec)
        {
            BMCWEB_LOG_ERROR << "Couldn't connect to nbd-proxy: "
                             << ec.message();
            if (session != nullptr)
            {
                session->close("Error connecting to nbd-proxy");
            }
            return;
        }
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

        if (inputBuffer.size() == 0)
        {
            BMCWEB_LOG_DEBUG << "inputBuffer empty.  Bailing out";
            return;
        }

        doingWrite = true;
        pipeIn.async_write_some(
            inputBuffer.data(),
            [this, self(shared_from_this())](boost::beast::error_code ec,
                                             std::size_t bytesWritten) {
                BMCWEB_LOG_DEBUG << "Wrote " << bytesWritten << "bytes";
                doingWrite = false;
                inputBuffer.consume(bytesWritten);

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
                    BMCWEB_LOG_ERROR << "Error in VM socket write " << ec;
                    return;
                }
                doWrite();
            });
    }

    void doRead()
    {
        std::size_t bytes = outputBuffer.capacity() - outputBuffer.size();

        pipeOut.async_read_some(
            outputBuffer.prepare(bytes),
            [this, self(shared_from_this())](
                const boost::system::error_code& ec, std::size_t bytesRead) {
                BMCWEB_LOG_DEBUG << "Read done.  Read " << bytesRead
                                 << " bytes";
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Couldn't read from VM port: " << ec;
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

                outputBuffer.commit(bytesRead);
                boost::beast::string_view payload(
                    static_cast<const char*>(outputBuffer.data().data()),
                    bytesRead);
                session->sendBinary(payload);
                outputBuffer.consume(bytesRead);

                doRead();
            });
    }

    boost::process::async_pipe pipeOut;
    boost::process::async_pipe pipeIn;
    boost::process::child proxy;
    std::string media;
};

std::shared_ptr<Handler> handler;

template <typename... Middlewares> void requestRoutes(Crow<Middlewares...>& app)
{
    BMCWEB_ROUTE(app, "/vm/0")
        .methods(
            "GET"_method)([](const crow::Request& req, crow::Response& res) {
            static constexpr auto config = "/etc/nbd-proxy/config.json";

            std::ifstream file(config);
            if (!file.good())
            {
                BMCWEB_LOG_ERROR
                    << "Error reading virtual media configuration file: "
                    << config;
                res.result(boost::beast::http::status::internal_server_error);
                res.end();
                return;
            }

            nlohmann::json data = nlohmann::json::parse(file, nullptr, false);
            if (data.is_discarded())
            {
                BMCWEB_LOG_ERROR << "Error parsing the json file for "
                                    "virtual media configurations";
                res.result(boost::beast::http::status::internal_server_error);
                res.end();
                return;
            }
            nlohmann::json::iterator j = data.find("configurations");
            if (j == data.end())
            {
                BMCWEB_LOG_ERROR << "No configurations found";
                res.result(boost::beast::http::status::internal_server_error);
                res.end();
                return;
            }

            nlohmann::json jsonData;
            for (const auto& item : j->items())
            {
                if (!item.value().is_object())
                {
                    BMCWEB_LOG_ERROR << "No configuration data found";
                    res.result(
                        boost::beast::http::status::internal_server_error);
                    res.end();
                    return;
                }

                nlohmann::json::iterator m = item.value().find("metadata");
                if (m == item.value().end())
                {
                    BMCWEB_LOG_ERROR << "No configuration metadata found";
                    res.result(
                        boost::beast::http::status::internal_server_error);
                    res.end();
                    return;
                }

                jsonData[item.key()] = std::move(*m);
            }
            res.jsonValue = {{"data", std::move(jsonData)},
                             {"message", "200 OK"},
                             {"status", "ok"}};
            res.end();
        });

    BMCWEB_ROUTE(app, "/vm/0/0")
        .websocket()
        .onopen([](crow::websocket::Connection& conn) {
            BMCWEB_LOG_DEBUG << "Connection " << &conn << " opened";

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
            handler = std::make_shared<Handler>(media, conn.get_io_context());
            handler->connect();
        })
        .onclose(
            [](crow::websocket::Connection& conn, const std::string& reason) {
                session = nullptr;
                handler->doClose();
                handler.reset();
#if BOOST_VERSION >= 107000
                inputBuffer.clear();
                outputBuffer.clear();
#else
                inputBuffer.reset();
                outputBuffer.reset();
#endif
            })
        .onmessage([](crow::websocket::Connection& conn,
                      const std::string& data, bool is_binary) {
            if (data.length() > inputBuffer.capacity())
            {
                BMCWEB_LOG_ERROR << "Buffer overrun when writing "
                                 << data.length() << " bytes";
                conn.close("Buffer overrun");
                return;
            }

            boost::asio::buffer_copy(inputBuffer.prepare(data.size()),
                                     boost::asio::buffer(data));
            inputBuffer.commit(data.size());
            handler->doWrite();
        });
}

} // namespace obmc_vm
} // namespace crow
