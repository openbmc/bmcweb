#pragma once
#include <crow/app.h>
#include <crow/websocket.h>
#include <sys/socket.h>

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
                for (auto session : sessions)
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
                for (auto session : sessions)
                {
                    session->close("Error in connecting to host port");
                }
                return;
            }
            boost::beast::string_view payload(outputBuffer.data(), bytesRead);
            for (auto session : sessions)
            {
                session->sendText(payload);
            }
            doRead();
        });
}

void connectHandler(const boost::system::error_code& ec)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR << "Couldn't connect to host serial port: " << ec;
        for (auto session : sessions)
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
        .websocket()
        .onopen([](crow::websocket::Connection& conn) {
            BMCWEB_LOG_DEBUG << "Connection " << &conn << " opened";

            sessions.insert(&conn);
            if (host_socket == nullptr)
            {
                const std::string consoleName("\0obmc-console", 13);
                boost::asio::local::stream_protocol::endpoint ep(consoleName);

                // This is a hack.  For whatever reason boost local endpoint has
                // a check to see if a string is null terminated, and if it is,
                // it drops the path character count by 1.  For abstract
                // sockets, we need the count to be the full sizeof(s->sun_path)
                // (ie 108), even though our path _looks_ like it's null
                // terminated.  This is likely a bug in asio that needs to be
                // submitted Todo(ed).  so the cheat here is to break the
                // abstraction for a minute, write a 1 to the last byte, this
                // causes the check at the end of resize here:
                // https://www.boost.org/doc/libs/1_68_0/boost/asio/local/detail/impl/endpoint.ipp
                // to not decrement us unesssesarily.
                struct sockaddr_un* s =
                    reinterpret_cast<sockaddr_un*>(ep.data());
                s->sun_path[sizeof(s->sun_path) - 1] = 1;
                ep.resize(sizeof(sockaddr_un));
                s->sun_path[sizeof(s->sun_path) - 1] = 0;

                host_socket = std::make_unique<
                    boost::asio::local::stream_protocol::socket>(
                    conn.getIoService());
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
