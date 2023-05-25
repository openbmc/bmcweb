#pragma once
#include "app.hpp"
#include "async_resp.hpp"
#include "websocket.hpp"

#include <sys/socket.h>

#include <boost/asio/local/stream_protocol.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>

namespace crow
{
namespace obmc_console
{

// Update this value each time we add new console route.
static constexpr const uint maxSessions = 32;

class ConsoleHandler : public std::enable_shared_from_this<ConsoleHandler>
{
  public:
    explicit ConsoleHandler(crow::websocket::Connection& connIn) :
        conn(connIn), hostSocket(conn.getIoContext())
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
            [this, self(shared_from_this())](
                const boost::system::error_code& ec, std::size_t bytesRead) {
            BMCWEB_LOG_DEBUG << "read done.  Read " << bytesRead << " bytes";

            if (ec)
            {
                BMCWEB_LOG_ERROR << "Couldn't read from host serial port: "
                                 << ec.message();
                conn.close("Error in connecting to host port");
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

    crow::websocket::Connection& conn;
    boost::beast::flat_static_buffer<4096UL> outputBuffer;
    std::string inputBuffer;
    boost::asio::local::stream_protocol::socket hostSocket;
    bool doingWrite{false};
};

using ObmcConsoleMap =
    boost::container::flat_map<crow::websocket::Connection*,
                               std::shared_ptr<ConsoleHandler>>;

inline ObmcConsoleMap& getConsoleHandlerMap()
{
    static ObmcConsoleMap consoleHandlerMap;
    return consoleHandlerMap;
}

// Create handler for the connection
inline std::shared_ptr<ConsoleHandler>
    addConnectionHandler(crow::websocket::Connection& conn)
{
    // Store the pair in the handlers map
    auto [iter, isNew] = getConsoleHandlerMap().emplace(
        &conn, std::make_shared<ConsoleHandler>(conn));
    std::shared_ptr<ConsoleHandler> handler = iter->second;

    BMCWEB_LOG_DEBUG << "Obmc handler " << handler << " added " << isNew
                     << " for path " << iter->first;

    return handler;
}

// Validate the connection and get the handler
inline std::shared_ptr<ConsoleHandler>
    getConsoleHandler(crow::websocket::Connection& conn)
{
    // Look up the handler
    auto iter = getConsoleHandlerMap().find(&conn);
    if (iter == getConsoleHandlerMap().end())
    {
        BMCWEB_LOG_ERROR << "Failed to find the handler";
        return nullptr;
    }

    std::shared_ptr<ConsoleHandler> handler = iter->second;

    return handler;
}

// Remove console handler for the connection
inline void removeConsoleHandler(crow::websocket::Connection& conn,
                                 const std::string& err)
{
    BMCWEB_LOG_INFO << "Closing websocket. Reason: " << err;

    auto iter = getConsoleHandlerMap().find(&conn);
    if (iter != getConsoleHandlerMap().end())
    {
        std::shared_ptr<ConsoleHandler> handler = iter->second;

        BMCWEB_LOG_DEBUG << "Remove connection " << iter->first
                         << " from obmc handler " << handler << " for path "
                         << conn.req.target();

        getConsoleHandlerMap().erase(iter);
    }
}

inline void connectConsoleSocket(crow::websocket::Connection& conn,
                                 const boost::system::error_code& ec,
                                 const sdbusplus::message::unix_fd& unixfd)
{
    int fd = -1;

    if (ec)
    {
        BMCWEB_LOG_ERROR << "Failed to call console Connect() method"
                         << " DBUS error: " << ec.message();
        removeConsoleHandler(conn, "Failed to call console Connect() method");
        return;
    }

    // Look up the handler
    std::shared_ptr<ConsoleHandler> handler = getConsoleHandler(conn);
    if (handler == nullptr)
    {
        BMCWEB_LOG_ERROR << "Failed to find the handler";
        removeConsoleHandler(conn, "Failed to find the handler");
        return;
    }

    fd = dup(unixfd);
    if (fd == -1)
    {
        BMCWEB_LOG_ERROR << "Failed to dup the DBUS unixfd"
                         << " error: " << strerror(errno);
        removeConsoleHandler(conn, "Failed to dup the DBUS unixfd");
        return;
    }

    BMCWEB_LOG_DEBUG << "Console web socket path: " << conn.req.target()
                     << " Console unix FD: " << unixfd << " duped FD: " << fd;

    if (!handler->connect(fd))
    {
        close(fd);
        removeConsoleHandler(conn, "Failed to assign the DBUS socket");
    }
}

// Query consoles from DBUS and find the matching to the
// rules string.
inline void onOpen(crow::websocket::Connection& conn)
{
    BMCWEB_LOG_DEBUG << "Connection " << &conn << " opened";

    if (getConsoleHandlerMap().size() >= maxSessions)
    {
        conn.close("Max sessions are already connected");
        return;
    }

    std::shared_ptr<ConsoleHandler> handler = addConnectionHandler(conn);
    if (handler == nullptr)
    {
        return;
    }

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

inline void onClose(crow::websocket::Connection& conn, const std::string& err)
{
    removeConsoleHandler(conn, err);
}

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/console0")
        .privileges({{"OpenBMCHostConsole"}})
        .websocket()
        .onopen(onOpen)
        .onclose(onClose)
        .onmessage([](crow::websocket::Connection& conn,
                      const std::string& data, [[maybe_unused]] bool isBinary) {
            std::shared_ptr<ConsoleHandler> handler = getConsoleHandler(conn);
            if (handler != nullptr)
            {
                handler->inputBuffer += data;
                handler->doWrite();
            }
        });
}
} // namespace obmc_console
} // namespace crow
