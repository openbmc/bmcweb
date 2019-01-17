#pragma once

#include "filesystem.hpp"

#include <crow/app.h>
#include <crow/websocket.h>
#include <signal.h>

#include <boost/process.hpp>
#include <nlohmann/json.hpp>
#include <webserver_common.hpp>

namespace crow
{
namespace obmc_vm
{

namespace filesystem = std::filesystem;

static crow::websocket::Connection* session = nullptr;

static std::array<char, 131072> outputBuffer;
static std::string inputBuffer;

static bool doingWrite = false;

class Handler
{
  public:
    Handler(const std::string& media, boost::asio::io_service& ios) :
        media(media), pipeOut(ios), pipeIn(ios)
    {
    }

    ~Handler()
    {
        // boost::process::child::terminate uses SIGKILL, need to send SIGTERM
        // to allow the proxy to stop nbd-client and the USB device gadget.
        auto rc = kill(proxy.id(), SIGTERM);
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

        if (inputBuffer.empty())
        {
            BMCWEB_LOG_DEBUG << "inputBuffer empty.  Bailing out";
            return;
        }

        doingWrite = true;
        pipeIn.async_write_some(
            boost::asio::buffer(inputBuffer.data(), inputBuffer.size()),
            [this](boost::beast::error_code ec, std::size_t bytes_written) {
                BMCWEB_LOG_DEBUG << "Wrote " << bytes_written << "bytes";
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
                    BMCWEB_LOG_ERROR << "Error in VM socket write " << ec;
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
                BMCWEB_LOG_DEBUG << "read done.  Read " << bytesRead
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

                boost::beast::string_view payload(outputBuffer.data(),
                                                  bytesRead);
                session->sendBinary(payload);
                doRead();
            });
    }

    boost::process::async_pipe pipeOut;
    boost::process::async_pipe pipeIn;
    boost::process::child proxy;
    std::string media;
};

std::unique_ptr<Handler> handler;

template <typename... Middlewares> void requestRoutes(Crow<Middlewares...>& app)
{
    BMCWEB_ROUTE(app, "/vm/0")
        .methods(
            "GET"_method)([](const crow::Request& req, crow::Response& res) {
            static constexpr auto config = "/etc/nbd-proxy/config.json";

            if (filesystem::exists(config))
            {
                std::ifstream file(config);
                auto data = nlohmann::json::parse(file, nullptr, false);
                if (data.is_discarded())
                {
                    BMCWEB_LOG_ERROR << "Error parsing the json file for "
                                        "virtual media configurations";
                    res.result(
                        boost::beast::http::status::internal_server_error);
                    res.end();
                    return;
                }

                nlohmann::json jsonData;
                for (const auto& item : data["configurations"].items())
                {
                    for (const auto& meta :
                         data["configurations"][item.key()]["metadata"].items())
                    {
                        jsonData[item.key()][meta.key()] = meta.value();
                    }
                }
                res.jsonValue = {{"data", std::move(jsonData)},
                                 {"message", "200 OK"},
                                 {"status", "ok"}};
            }
            else
            {
                BMCWEB_LOG_ERROR << "Missing virtual media configuration file: "
                                 << config;
                res.result(boost::beast::http::status::internal_server_error);
                res.end();
                return;
            }
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

            session = &conn;

            if (handler != nullptr)
            {
                conn.close("Handler already running");
                return;
            }

            // TODO Support /vm/0/<media> in the endpoint path, hardcode for now
            auto media = "0"; // <media>
            handler = std::make_unique<Handler>(media, conn.get_io_context());
            handler->connect();
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
