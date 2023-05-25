#pragma once
#include "app.hpp"
#include "async_resp.hpp"
#include "websocket.hpp"

#include <sys/socket.h>

#include <boost/asio/local/stream_protocol.hpp>
#include <boost/container/flat_map.hpp>

namespace crow
{
namespace obmc_console
{

class ConsoleHandler : public std::enable_shared_from_this<ConsoleHandler>
{
  public:
    ConsoleHandler(boost::asio::io_context& ioc,
                   crow::websocket::Connection& connIn) :
        hostSocket(ioc),
        conn(connIn)
    {}

    ~ConsoleHandler() = default;

    ConsoleHandler(const ConsoleHandler&) = delete;
    ConsoleHandler(ConsoleHandler&&) = delete;
    ConsoleHandler& operator=(const ConsoleHandler&) = delete;
    ConsoleHandler& operator=(ConsoleHandler&&) = delete;

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
        hostSocket.async_write_some(
            boost::asio::buffer(inputBuffer.data(), inputBuffer.size()),
            [weak(weak_from_this())](const boost::beast::error_code& ec,
                                     std::size_t bytesWritten) {
            std::shared_ptr<ConsoleHandler> self = weak.lock();
            if (self == nullptr)
            {
                return;
            }

            self->doingWrite = false;
            self->inputBuffer.erase(0, bytesWritten);

            if (ec == boost::asio::error::eof)
            {
                self->conn.close("Error in reading to host port");
                return;
            }
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Error in host serial write "
                                 << ec.message();
                return;
            }
            self->doWrite();
            });
    }

    void doRead()
    {
        std::size_t bytes = outputBuffer.capacity() - outputBuffer.size();

        BMCWEB_LOG_DEBUG << "Reading from socket";
        hostSocket.async_read_some(
            outputBuffer.prepare(bytes),
            [this, weakSelf(weak_from_this())](
                const boost::system::error_code& ec, std::size_t bytesRead) {
            BMCWEB_LOG_DEBUG << "read done.  Read " << bytesRead << " bytes";
            std::shared_ptr<ConsoleHandler> self = weakSelf.lock();
            if (self == nullptr)
            {
                return;
            }
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Couldn't read from host serial port: "
                                 << ec.message();
                conn.close("Error connecting to host port");
                return;
            }
            outputBuffer.commit(bytesRead);
            std::string_view payload(
                static_cast<const char*>(outputBuffer.data().data()),
                bytesRead);
            conn.sendBinary(payload);
            outputBuffer.consume(bytesRead);
            doRead();
            });
    }

    bool connect(int fd)
    {
        boost::system::error_code ec;
        boost::asio::local::stream_protocol proto;

        hostSocket.assign(proto, fd, ec);

        if (ec)
        {
            BMCWEB_LOG_ERROR << "Failed to assign the DBUS socket"
                             << " Socket assign error: " << ec.message();
            return false;
        }

        conn.resumeRead();
        doWrite();
        doRead();
        return true;
    }

    boost::asio::local::stream_protocol::socket hostSocket;

    boost::beast::flat_static_buffer<4096> outputBuffer;

    std::string inputBuffer;
    bool doingWrite = false;
    crow::websocket::Connection& conn;
};

using ObmcConsoleMap = boost::container::flat_map<
    crow::websocket::Connection*, std::shared_ptr<ConsoleHandler>, std::less<>,
    std::vector<std::pair<crow::websocket::Connection*,
                          std::shared_ptr<ConsoleHandler>>>>;

inline ObmcConsoleMap& getConsoleHandlerMap()
{
    static ObmcConsoleMap map;
    return map;
}

// Remove connection from the connection map and if connection map is empty
// then remove the handler from handlers map.
inline void onClose(crow::websocket::Connection& conn, const std::string& err)
{
    BMCWEB_LOG_INFO << "Closing websocket. Reason: " << err;

    auto iter = getConsoleHandlerMap().find(&conn);
    if (iter == getConsoleHandlerMap().end())
    {
        BMCWEB_LOG_CRITICAL << "Unable to find connection " << &conn;
        return;
    }
    BMCWEB_LOG_DEBUG << "Remove connection " << &conn << " from obmc console";

    // Removed last connection so remove the path
    getConsoleHandlerMap().erase(iter);
}

inline void connectConsoleSocket(crow::websocket::Connection& conn,
                                 const boost::system::error_code& ec,
                                 const sdbusplus::message::unix_fd& unixfd)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR << "Failed to call console Connect() method"
                         << " DBUS error: " << ec.message();
        conn.close("Failed to connect");
        return;
    }

    // Look up the handler
    auto iter = getConsoleHandlerMap().find(&conn);
    if (iter == getConsoleHandlerMap().end())
    {
        BMCWEB_LOG_ERROR << "Failed to find the handler";
        conn.close("Internal error");
        return;
    }

    int fd = dup(unixfd);
    if (fd == -1)
    {
        BMCWEB_LOG_ERROR << "Failed to dup the DBUS unixfd"
                         << " error: " << strerror(errno);
        conn.close("Internal error");
        return;
    }

    BMCWEB_LOG_DEBUG << "Console web socket path: " << conn.req.target()
                     << " Console unix FD: " << unixfd << " duped FD: " << fd;

    if (!iter->second->connect(fd))
    {
        close(fd);
        conn.close("Internal Error");
    }
}

// Query consoles from DBUS and find the matching to the
// rules string.
inline void onOpen(crow::websocket::Connection& conn)
{
    BMCWEB_LOG_DEBUG << "Connection " << &conn << " opened";

    std::shared_ptr<ConsoleHandler> handler =
        std::make_shared<ConsoleHandler>(conn.getIoContext(), conn);
    getConsoleHandlerMap().emplace(&conn, handler);

    conn.deferRead();

    // The console id 'default' is used for the console0
    // We need to change it when we provide full multi-console support.
    const std::string consolePath = "/xyz/openbmc_project/console/default";
    const std::string consoleService = "xyz.openbmc_project.Console.default";

    BMCWEB_LOG_DEBUG << "Console Object path = " << consolePath
                     << " service = " << consoleService
                     << " Request target = " << conn.req.target();

    // Call Connect() method to get the unix FD
    crow::connections::systemBus->async_method_call(
        [&conn](const boost::system::error_code& ec,
                const sdbusplus::message::unix_fd& unixfd) {
        connectConsoleSocket(conn, ec, unixfd);
        },
        consoleService, consolePath, "xyz.openbmc_project.Console.Access",
        "Connect");
}

inline void onMessage(crow::websocket::Connection& conn,
                      const std::string& data, bool /*isBinary*/)
{
    auto handler = getConsoleHandlerMap().find(&conn);
    if (handler == getConsoleHandlerMap().end())
    {
        BMCWEB_LOG_CRITICAL << "Unable to find connection " << &conn;
        return;
    }
    handler->second->inputBuffer += data;
    handler->second->doWrite();
}

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/console0")
        .privileges({{"OpenBMCHostConsole"}})
        .websocket()
        .onopen(onOpen)
        .onclose(onClose)
        .onmessage(onMessage);
}
} // namespace obmc_console
} // namespace crow
