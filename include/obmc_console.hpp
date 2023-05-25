#pragma once
#include "app.hpp"
#include "async_resp.hpp"
#include "websocket.hpp"

#include <sys/socket.h>

#include <boost/asio/local/stream_protocol.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/container/map.hpp>

namespace crow
{
namespace obmc_console
{

class obmc_handler : public std::enable_shared_from_this<obmc_handler>
{
  public:
    obmc_handler() {}

    ~obmc_handler() = default;

    obmc_handler(const obmc_handler&) = delete;
    obmc_handler(obmc_handler&&) = delete;
    obmc_handler& operator=(const obmc_handler&) = delete;
    obmc_handler& operator=(obmc_handler&&) = delete;

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
            [this](const boost::beast::error_code& ec,
                   std::size_t bytesWritten) {
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
                BMCWEB_LOG_ERROR << "Error in host serial write "
                                 << ec.message();
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
            [this](const boost::system::error_code& ec, std::size_t bytesRead) {
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

    void addConnection(crow::websocket::Connection& conn)
    {
        sessions.insert(&conn);
    }

    // Removes true if session table is empty.
    bool removeConnection(crow::websocket::Connection& conn,
                          const std::string& err)
    {
        if (sessions.erase(&conn))
        {
            conn.close(err);
        }
        return sessions.empty();
    }

    bool connect(crow::websocket::Connection& conn, int fd)
    {
        boost::system::error_code ec;
        boost::asio::local::stream_protocol proto;
        hostSocket =
            std::make_unique<boost::asio::local::stream_protocol::socket>(
                conn.getIoContext());

        hostSocket->assign(proto, fd, ec);

        if (ec)
        {
            BMCWEB_LOG_ERROR << "Failed to assign the DBUS socket"
                             << " Socket assign error: " << ec.message();
            return false;
        }
        else
        {
            conn.resumeRead();
            doWrite();
            doRead();
            return true;
        }
    }

    std::unique_ptr<boost::asio::local::stream_protocol::socket> hostSocket;
    std::array<char, 4096> outputBuffer;
    std::string inputBuffer;
    bool doingWrite = false;
    boost::container::flat_set<crow::websocket::Connection*> sessions;
};

// The map contains routing path as a key and obmc_handler pointer as data.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static boost::container::flat_map<std::string, std::shared_ptr<obmc_handler>>
    obmcHandlers;

// Create handler for the route on first connection and add connection in the
// connection map.
inline std::shared_ptr<obmc_handler>
    addConnectionAndHandler(crow::websocket::Connection& conn)
{
    // Store the pair in the handlers map
    auto [iter, isNew] = obmcHandlers.try_emplace(
        conn.req.target(), std::make_shared<obmc_handler>());
    std::shared_ptr<obmc_handler> handler = iter->second;

    BMCWEB_LOG_DEBUG << "Obmc handler " << handler << " added " << isNew
                     << " for path " << iter->first;

    // Save this connection in the map
    handler->addConnection(conn);

    return handler;
}

// Remove connection from the connection map and if connection map is empty
// then remove the handler from handlers map.
inline void removeConnectionAndHandler(crow::websocket::Connection& conn,
                                       const std::string& err)
{
    const auto iter = obmcHandlers.find(conn.req.target());
    if (iter != obmcHandlers.end())
    {
        auto handler = iter->second;

        BMCWEB_LOG_DEBUG << "Remove connection " << &conn
                         << " from obmc handler " << handler << " for path "
                         << iter->first;

        if (handler->removeConnection(conn, err))
        {
            BMCWEB_LOG_DEBUG << "Remove obmc handler " << handler;

            // Removed last connection so remove the path
            obmcHandlers.erase(iter);
        }
    }
}

inline void connectConsoleSocket(std::shared_ptr<obmc_handler> handler,
                                 crow::websocket::Connection& conn,
                                 const boost::system::error_code& ec,
                                 const sdbusplus::message::unix_fd& unixfd)
{
    int fd = -1;

    if (ec)
    {
        BMCWEB_LOG_ERROR << "Failed to call console Connect() method"
                         << " DBUS error: " << ec.message();
        removeConnectionAndHandler(conn,
                                   "Failed to call console Connect() method");
        return;
    }

    // Make sure that connection is still open.
    if (!handler->sessions.contains(&conn))
    {
        return;
    }

    fd = dup(unixfd);
    if (fd == -1)
    {
        BMCWEB_LOG_ERROR << "Failed to dup the DBUS unixfd"
                         << " error: " << strerror(errno);
        removeConnectionAndHandler(conn, "Failed to dup the DBUS unixfd");
        return;
    }

    BMCWEB_LOG_DEBUG << "Console web socket path: " << conn.req.target()
                     << " Console unix FD: " << unixfd << " duped FD: " << fd;

    if (handler->hostSocket == nullptr)
    {
        if (!handler->connect(conn, fd))
        {
            close(fd);
            removeConnectionAndHandler(conn,
                                       "Failed to assign the DBUS socket");
        }
    }
    else
    {
        BMCWEB_LOG_DEBUG << "Socket already exist so close the new fd: " << fd;
        close(fd);
    }
}

// Query consoles from DBUS and find the matching to the
// rules string.
inline void onOpen(crow::websocket::Connection& conn)
{
    BMCWEB_LOG_DEBUG << "Connection " << &conn << " opened";

    auto handler = addConnectionAndHandler(conn);
    if (handler == nullptr)
    {
        return;
    }

    // We need to wait for dbus and the websockets to hook up before data is
    // sent/received.  Tell the core to hold off messages until the sockets are
    // up
    if (handler->hostSocket == nullptr)
    {
        conn.deferRead();
    }
    else
    {
        // Socket is already connected so no further action is required.
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
        [&handler, &conn](const boost::system::error_code& ec,
                          const sdbusplus::message::unix_fd& unixfd) {
        connectConsoleSocket(handler, conn, ec, unixfd);
        },
        consoleService, consolePath, "xyz.openbmc_project.Console.Access",
        "Connect");
}

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/console0")
        .privileges({{"OpenBMCHostConsole"}})
        .websocket()
        .onopen(onOpen)
        .onclose([](crow::websocket::Connection& conn,
                    [[maybe_unused]] const std::string& reason) {
            BMCWEB_LOG_INFO << "Closing websocket. Reason: " << reason;

            removeConnectionAndHandler(conn, reason);
        })
        .onmessage([]([[maybe_unused]] crow::websocket::Connection& conn,
                      const std::string& data, [[maybe_unused]] bool isBinary) {
            // Is handler loopup on each message is costly?
            auto iter = obmcHandlers.find(conn.req.target());
            if (iter != obmcHandlers.end())
            {
                auto handler = iter->second;
                handler->inputBuffer += data;
                handler->doWrite();
            }
        });
}
} // namespace obmc_console
} // namespace crow
