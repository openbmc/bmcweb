#pragma once
#include "app.hpp"
#include "async_resp.hpp"
#include "websocket.hpp"

#include <sys/socket.h>

#include <boost/asio/local/stream_protocol.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/system/error_code.hpp>

#include <array>
#include <memory>
#include <string>
#include <string_view>

namespace crow
{
namespace obmc_console
{

// Update this value each time we add new console route.
static constexpr const uint maxSessions = 32;

class ConsoleHandler : public std::enable_shared_from_this<ConsoleHandler>
{
  public:
    ConsoleHandler(boost::asio::io_context& ioc,
                   crow::websocket::Connection& connIn) :
        hostSocket(ioc), conn(connIn)
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
            BMCWEB_LOG_DEBUG("Already writing.  Bailing out");
            return;
        }

        if (inputBuffer.empty())
        {
            BMCWEB_LOG_DEBUG("Outbuffer empty.  Bailing out");
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
                BMCWEB_LOG_ERROR("Error in host serial write {}", ec.message());
                return;
            }
            self->doWrite();
        });
    }

    static void afterSendEx(const std::weak_ptr<ConsoleHandler>& weak)
    {
        std::shared_ptr<ConsoleHandler> self = weak.lock();
        if (self == nullptr)
        {
            return;
        }
        self->doRead();
    }

    void doRead()
    {
        BMCWEB_LOG_DEBUG("Reading from socket");
        hostSocket.async_read_some(
            boost::asio::buffer(outputBuffer),
            [this, weakSelf(weak_from_this())](
                const boost::system::error_code& ec, std::size_t bytesRead) {
            BMCWEB_LOG_DEBUG("read done.  Read {} bytes", bytesRead);
            std::shared_ptr<ConsoleHandler> self = weakSelf.lock();
            if (self == nullptr)
            {
                return;
            }
            if (ec)
            {
                BMCWEB_LOG_ERROR("Couldn't read from host serial port: {}",
                                 ec.message());
                conn.close("Error connecting to host port");
                return;
            }
            std::string_view payload(outputBuffer.data(), bytesRead);
            self->conn.sendEx(crow::websocket::MessageType::Binary, payload,
                              std::bind_front(afterSendEx, weak_from_this()));
        });
    }

    bool connect(int fd)
    {
        boost::system::error_code ec;
        boost::asio::local::stream_protocol proto;

        hostSocket.assign(proto, fd, ec);

        if (ec)
        {
            BMCWEB_LOG_ERROR(
                "Failed to assign the DBUS socket Socket assign error: {}",
                ec.message());
            return false;
        }

        conn.resumeRead();
        doWrite();
        doRead();
        return true;
    }

    boost::asio::local::stream_protocol::socket hostSocket;

    std::array<char, 4096> outputBuffer{};

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
    BMCWEB_LOG_INFO("Closing websocket. Reason: {}", err);

    auto iter = getConsoleHandlerMap().find(&conn);
    if (iter == getConsoleHandlerMap().end())
    {
        BMCWEB_LOG_CRITICAL("Unable to find connection {}", logPtr(&conn));
        return;
    }
    BMCWEB_LOG_DEBUG("Remove connection {} from obmc console", logPtr(&conn));

    // Removed last connection so remove the path
    getConsoleHandlerMap().erase(iter);
}

inline void connectConsoleSocket(crow::websocket::Connection& conn,
                                 const boost::system::error_code& ec,
                                 const sdbusplus::message::unix_fd& unixfd)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR(
            "Failed to call console Connect() method DBUS error: {}",
            ec.message());
        conn.close("Failed to connect");
        return;
    }

    // Look up the handler
    auto iter = getConsoleHandlerMap().find(&conn);
    if (iter == getConsoleHandlerMap().end())
    {
        BMCWEB_LOG_ERROR("Connection was already closed");
        return;
    }

    int fd = dup(unixfd);
    if (fd == -1)
    {
        BMCWEB_LOG_ERROR("Failed to dup the DBUS unixfd error: {}",
                         strerror(errno));
        conn.close("Internal error");
        return;
    }

    BMCWEB_LOG_DEBUG("Console duped FD: {}", fd);

    if (!iter->second->connect(fd))
    {
        close(fd);
        conn.close("Internal Error");
    }
}

inline void
    processConsoleObject(crow::websocket::Connection& conn,
                         const std::string& consoleObjPath,
                         const boost::system::error_code& ec,
                         const ::dbus::utility::MapperGetObject& objInfo)
{
    // Look up the handler
    auto iter = getConsoleHandlerMap().find(&conn);
    if (iter == getConsoleHandlerMap().end())
    {
        BMCWEB_LOG_ERROR("Connection was already closed");
        return;
    }

    if (ec)
    {
        BMCWEB_LOG_WARNING("getDbusObject() for consoles failed. DBUS error:{}",
                           ec.message());
        conn.close("getDbusObject() for consoles failed.");
        return;
    }

    const auto valueIface = objInfo.begin();
    if (valueIface == objInfo.end())
    {
        BMCWEB_LOG_WARNING("getDbusObject() returned unexpected size: {}",
                           objInfo.size());
        conn.close("getDbusObject() returned unexpected size");
        return;
    }

    const std::string& consoleService = valueIface->first;
    BMCWEB_LOG_DEBUG("Looking up unixFD for Service {} Path {}", consoleService,
                     consoleObjPath);
    // Call Connect() method to get the unix FD
    crow::connections::systemBus->async_method_call(
        [&conn](const boost::system::error_code& ec1,
                const sdbusplus::message::unix_fd& unixfd) {
        connectConsoleSocket(conn, ec1, unixfd);
    },
        consoleService, consoleObjPath, "xyz.openbmc_project.Console.Access",
        "Connect");
}

// Query consoles from DBUS and find the matching to the
// rules string.
inline void onOpen(crow::websocket::Connection& conn)
{
    std::string consoleLeaf;

    BMCWEB_LOG_DEBUG("Connection {} opened", logPtr(&conn));

    if (getConsoleHandlerMap().size() >= maxSessions)
    {
        conn.close("Max sessions are already connected");
        return;
    }

    std::shared_ptr<ConsoleHandler> handler =
        std::make_shared<ConsoleHandler>(conn.getIoContext(), conn);
    getConsoleHandlerMap().emplace(&conn, handler);

    conn.deferRead();

    // Keep old path for backward compatibility
    if (conn.url().path() == "/console0")
    {
        consoleLeaf = "default";
    }
    else
    {
        // Get the console id from console router path and prepare the console
        // object path and console service.
        consoleLeaf = conn.url().segments().back();
    }
    std::string consolePath =
        sdbusplus::message::object_path("/xyz/openbmc_project/console") /
        consoleLeaf;

    BMCWEB_LOG_DEBUG("Console Object path = {} Request target = {}",
                     consolePath, conn.url().path());

    // mapper call lambda
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Console.Access"};

    dbus::utility::getDbusObject(
        consolePath, interfaces,
        [&conn, consolePath](const boost::system::error_code& ec,
                             const ::dbus::utility::MapperGetObject& objInfo) {
        processConsoleObject(conn, consolePath, ec, objInfo);
    });
}

inline void onMessage(crow::websocket::Connection& conn,
                      const std::string& data, bool /*isBinary*/)
{
    auto handler = getConsoleHandlerMap().find(&conn);
    if (handler == getConsoleHandlerMap().end())
    {
        BMCWEB_LOG_CRITICAL("Unable to find connection {}", logPtr(&conn));
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

    BMCWEB_ROUTE(app, "/console/<str>")
        .privileges({{"OpenBMCHostConsole"}})
        .websocket()
        .onopen(onOpen)
        .onclose(onClose)
        .onmessage(onMessage);
}
} // namespace obmc_console
} // namespace crow
