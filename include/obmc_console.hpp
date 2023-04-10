#pragma once
#include "app.hpp"
#include "async_resp.hpp"

#include <sys/socket.h>

#include <boost/asio/local/stream_protocol.hpp>
#include <boost/container/flat_set.hpp>
#include <websocket.hpp>

namespace crow
{
namespace obmc_console
{

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static std::unique_ptr<boost::asio::local::stream_protocol::socket> hostSocket;

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static std::array<char, 4096> outputBuffer;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static std::string inputBuffer;

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static boost::container::flat_set<crow::websocket::Connection*> sessions;

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static bool doingWrite = false;

inline void doWrite()
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

    if (!hostSocket)
    {
        BMCWEB_LOG_ERROR << "doWrite(): Socket closed.";
        return;
    }

    doingWrite = true;
    hostSocket->async_write_some(
        boost::asio::buffer(inputBuffer.data(), inputBuffer.size()),
        [](boost::beast::error_code ec, std::size_t bytesWritten) {
        doingWrite = false;
        inputBuffer.erase(0, bytesWritten);

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
            BMCWEB_LOG_ERROR << "Error in host serial write " << ec.message();
            return;
        }
        doWrite();
        });
}

inline void doRead()
{
    if (!hostSocket)
    {
        BMCWEB_LOG_ERROR << "doRead(): Socket closed.";
        return;
    }

    BMCWEB_LOG_DEBUG << "Reading from socket";
    hostSocket->async_read_some(
        boost::asio::buffer(outputBuffer.data(), outputBuffer.size()),
        [](const boost::system::error_code& ec, std::size_t bytesRead) {
        BMCWEB_LOG_DEBUG << "read done.  Read " << bytesRead << " bytes";
        if (ec)
        {
            BMCWEB_LOG_ERROR << "Couldn't read from host serial port: "
                             << ec.message();
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

// If connection is active then remove it from the connection map
inline bool removeConnection(crow::websocket::Connection& conn)
{
    int ret = false;

    if (sessions.erase(&conn))
        ret = true;

    if (sessions.empty())
    {
        hostSocket = nullptr;
        inputBuffer.clear();
        inputBuffer.shrink_to_fit();
    }
    return ret;
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
        if (removeConnection(conn))
        {
            conn.close("Failed to call console Connect() method");
        }
        return;
    }

    // Make sure that connection is still open.
    if (!sessions.contains(&conn))
    {
        return;
    }

    fd = dup(unixfd);
    if (fd == -1)
    {
        BMCWEB_LOG_ERROR << "Failed to dup the DBUS unixfd"
                         << " error: " << strerror(errno);
        if (removeConnection(conn))
        {
            conn.close("Failed to dup the DBUS unixfd");
        }
        return;
    }

    BMCWEB_LOG_DEBUG << "Console web socket path: " << conn.req.target()
                     << " Console unix FD: " << unixfd << " duped FD: " << fd;

    if (hostSocket == nullptr)
    {
        boost::system::error_code ec1;
        boost::asio::local::stream_protocol proto;
        hostSocket =
            std::make_unique<boost::asio::local::stream_protocol::socket>(
                conn.getIoContext());

        hostSocket->assign(proto, fd, ec1);

        if (ec1)
        {
            BMCWEB_LOG_ERROR << "Failed to assign the DBUS socket"
                             << " Socket assign error: " << ec1.message();
            if (removeConnection(conn))
            {
                conn.close("Failed to assign the DBUS socket");
            }
        }
        else
        {
            conn.resumeRead();
            doWrite();
            doRead();
        }
    }
    else
    {
        BMCWEB_LOG_DEBUG << "Socket already exist so close the new fd: " << fd;
        close(fd);
    }
}

inline void
    processConsoleObject(crow::websocket::Connection& conn,
                         const std::string& consoleObjPath,
                         const boost::system::error_code& ec,
                         const ::dbus::utility::MapperGetObject& objInfo)
{
    // Make sure that connection is still open.
    if (!sessions.contains(&conn))
    {
        return;
    }

    if (ec)
    {
        BMCWEB_LOG_WARNING << "getDbusObject() for consoles failed. DBUS error:"
                           << ec.message();
        if (removeConnection(conn))
        {
            conn.close("getDbusObject() for consoles failed.");
        }
        return;
    }

    if (objInfo.size() != 1)
    {
        BMCWEB_LOG_WARNING << "getDbusObject() returned unexpected size: "
                           << objInfo.size();
        if (removeConnection(conn))
        {
            conn.close("getDbusObject() returned unexpected size");
        }
        return;
    }

    const auto& valueIface = *objInfo.begin();
    const std::string& consoleService = valueIface.first;
    BMCWEB_LOG_DEBUG << "Looking up unixFD for Service " << consoleService
                     << " Path " << consoleObjPath;

    // Call Connect() method to get the unix FD
    crow::connections::systemBus->async_method_call(
        [&conn](const boost::system::error_code& ec1,
                const sdbusplus::message::unix_fd& unixfd) mutable {
        connectConsoleSocket(conn, ec1, unixfd);
        },
        consoleService, consoleObjPath, "xyz.openbmc_project.Console.Access",
        "Connect");
}

// Query consoles from DBUS and find the matching to the
// rules string.
inline void onOpen(crow::websocket::Connection& conn)
{
    BMCWEB_LOG_DEBUG << "Connection " << &conn << " opened";

    // Save the connection in the map
    sessions.insert(&conn);

    // We need to wait for dbus and the websockets to hook up before data is
    // sent/received.  Tell the core to hold off messages until the sockets are
    // up
    if (hostSocket == nullptr)
    {
        conn.deferRead();
    }

    // mapper call lambda
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Console.Access"};

    sdbusplus::message::object_path objPath(conn.req.target());
    std::string consolePath =
        "/xyz/openbmc_project/console/" + objPath.filename();

    BMCWEB_LOG_DEBUG << "Console Object path = " << consolePath
                     << " Request target = " << conn.req.target();

    dbus::utility::getDbusObject(
        consolePath, interfaces,
        [&conn, consolePath](const boost::system::error_code& ec,
                             const ::dbus::utility::MapperGetObject& objInfo) {
        processConsoleObject(conn, consolePath, ec, objInfo);
        });
}

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/console0")
        .privileges({{"ConfigureComponents", "ConfigureManager"}})
        .websocket()
        .onopen(onOpen)
        .onclose([](crow::websocket::Connection& conn,
                    [[maybe_unused]] const std::string& reason) {
            BMCWEB_LOG_INFO << "Closing websocket. Reason: " << reason;

            removeConnection(conn);
        })
        .onmessage([]([[maybe_unused]] crow::websocket::Connection& conn,
                      const std::string& data, [[maybe_unused]] bool isBinary) {
            inputBuffer += data;
            doWrite();
        });
}
} // namespace obmc_console
} // namespace crow
