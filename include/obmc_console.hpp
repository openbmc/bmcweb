#pragma once
#include <app.h>
#include <sys/socket.h>
#include <websocket.h>

#include <async_resp.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <webserver_common.hpp>

namespace crow
{
namespace obmc_console
{

static std::unique_ptr<boost::asio::local::stream_protocol::socket> host_socket;

static std::array<char, 4096> outputBuffer;
static std::string inputBuffer;

static boost::container::flat_set<crow::websocket::Connection*> sessions;

static bool doingWrite = false;

void doWrite()
{
    if (doingWrite)
    {
        BMCWEB_LOG_DEBUG << "Already writing.  Bailing out";
        return;
    }

    if (inputBuffer.empty())
    {
        BMCWEB_LOG_DEBUG << "Outbuffer empty.  Bailing out";
        return;
    }

    doingWrite = true;
    host_socket->async_write_some(
        boost::asio::buffer(inputBuffer.data(), inputBuffer.size()),
        [](boost::beast::error_code ec, std::size_t bytes_written) {
            doingWrite = false;
            inputBuffer.erase(0, bytes_written);

            if (ec == boost::asio::error::eof)
            {
                for (crow::websocket::Connection* session : sessions)
                {
                    session->close("Error in reading to host port");
                }
                return;
            }
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Error in host serial write " << ec;
                return;
            }
            doWrite();
        });
}

void doRead()
{
    BMCWEB_LOG_DEBUG << "Reading from socket";
    host_socket->async_read_some(
        boost::asio::buffer(outputBuffer.data(), outputBuffer.size()),
        [](const boost::system::error_code& ec, std::size_t bytesRead) {
            BMCWEB_LOG_DEBUG << "read done.  Read " << bytesRead << " bytes";
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Couldn't read from host serial port: "
                                 << ec;
                for (crow::websocket::Connection* session : sessions)
                {
                    session->close("Error in connecting to host port");
                }
                return;
            }
            std::string_view payload(outputBuffer.data(), bytesRead);
            for (crow::websocket::Connection* session : sessions)
            {
                session->sendBinary(payload);
            }
            doRead();
        });
}

void connectHandler(const boost::system::error_code& ec)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR << "Couldn't connect to host serial port: " << ec;
        for (crow::websocket::Connection* session : sessions)
        {
            session->close("Error in connecting to host port");
        }
        return;
    }

    doWrite();
    doRead();
}

void requestRoutes(CrowApp& app)
{
    BMCWEB_ROUTE(app, "/console0")
        .requires({"ConfigureComponents", "ConfigureManager"})
        .websocket()
        .onopen([](crow::websocket::Connection& conn,
                   std::shared_ptr<bmcweb::AsyncResp> asyncResp) {
            BMCWEB_LOG_DEBUG << "Connection " << &conn << " opened";

            sessions.insert(&conn);
            if (host_socket == nullptr)
            {
                const std::string consoleName("\0obmc-console", 13);
                boost::asio::local::stream_protocol::endpoint ep(consoleName);

                host_socket = std::make_unique<
                    boost::asio::local::stream_protocol::socket>(
                    conn.get_io_context());
                host_socket->async_connect(ep, connectHandler);
            }
        })
        .onclose(
            [](crow::websocket::Connection& conn, const std::string& reason) {
                sessions.erase(&conn);
                if (sessions.empty())
                {
                    host_socket = nullptr;
                    inputBuffer.clear();
                    inputBuffer.shrink_to_fit();
                }
            })
        .onmessage([](crow::websocket::Connection& conn,
                      const std::string& data, bool is_binary) {
            inputBuffer += data;
            doWrite();
        });
}
} // namespace obmc_console
} // namespace crow
