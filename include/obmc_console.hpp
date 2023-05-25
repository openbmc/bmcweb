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

class ObmcHandler : public std::enable_shared_from_this<ObmcHandler>
{
  public:
    ObmcHandler() = default;

    ~ObmcHandler() = default;

    ObmcHandler(const ObmcHandler&) = delete;
    ObmcHandler(ObmcHandler&&) = delete;
    ObmcHandler& operator=(const ObmcHandler&) = delete;
    ObmcHandler& operator=(ObmcHandler&&) = delete;

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
            [weak(weak_from_this())](const boost::beast::error_code& ec,
                                     std::size_t bytesWritten) {
            std::shared_ptr<ObmcHandler> self = weak.lock();
            if (self == nullptr)
            {
                return;
            }

            self->doingWrite = false;
            self->inputBuffer.erase(0, bytesWritten);

            if (ec == boost::asio::error::eof)
            {
                for (crow::websocket::Connection* session : self->sessions)
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
            self->doWrite();
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
            [weak(weak_from_this())](const boost::system::error_code& ec,
                                     std::size_t bytesRead) {
            BMCWEB_LOG_DEBUG << "read done.  Read " << bytesRead << " bytes";

            std::shared_ptr<ObmcHandler> self = weak.lock();
            if (self == nullptr)
            {
                return;
            }

            if (ec)
            {
                BMCWEB_LOG_ERROR << "Couldn't read from host serial port: "
                                 << ec.message();
                for (crow::websocket::Connection* session : self->sessions)
                {
                    session->close("Error in connecting to host port");
                }
                return;
            }
            std::string_view payload(self->outputBuffer.data(), bytesRead);
            for (crow::websocket::Connection* session : self->sessions)
            {
                session->sendBinary(payload);
            }
            self->doRead();
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
        if (sessions.erase(&conn) != 0U)
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

        conn.resumeRead();
        doWrite();
        doRead();
        return true;
    }

    std::unique_ptr<boost::asio::local::stream_protocol::socket> hostSocket;
    std::array<char, 4096> outputBuffer;
    std::string inputBuffer;
    bool doingWrite = false;
    boost::container::flat_set<crow::websocket::Connection*> sessions;
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static boost::container::flat_map<std::string, std::shared_ptr<ObmcHandler>>
    obmcHandlerMap;

// Create handler for the route on first connection and add connection in the
// connection map.
inline std::shared_ptr<ObmcHandler>
    addConnectionAndHandler(crow::websocket::Connection& conn)
{
    // Store the pair in the handlers map
    auto [iter, isNew] = obmcHandlerMap.try_emplace(
        conn.req.target(), std::make_shared<ObmcHandler>());
    std::shared_ptr<ObmcHandler> handler = iter->second;

    BMCWEB_LOG_DEBUG << "Obmc handler " << handler << " added " << isNew
                     << " for path " << iter->first;

    // Save this connection in the map
    handler->addConnection(conn);

    return handler;
}

// Validate the connection and get the handler
inline std::shared_ptr<ObmcHandler>
    getObmcHandler(crow::websocket::Connection& conn)
{
    // Look up the handler
    auto iter = obmcHandlerMap.find(conn.req.target());
    if (iter == obmcHandlerMap.end())
    {
        BMCWEB_LOG_ERROR << "Failed to find the handler";
        return nullptr;
    }

    std::shared_ptr<ObmcHandler> handler = iter->second;

    // Make sure that connection is still open.
    if (!handler->sessions.contains(&conn))
    {
        return nullptr;
    }

    return handler;
}

// Remove connection from the connection map and if connection map is empty
// then remove the handler from handlers map.
inline void removeConnectionAndHandler(crow::websocket::Connection& conn,
                                       const std::string& err)
{
    BMCWEB_LOG_INFO << "Closing websocket. Reason: " << err;

    auto iter = obmcHandlerMap.find(conn.req.target());
    if (iter != obmcHandlerMap.end())
    {
        std::shared_ptr<ObmcHandler> handler = iter->second;

        BMCWEB_LOG_DEBUG << "Remove connection " << &conn
                         << " from obmc handler " << handler << " for path "
                         << iter->first;

        if (handler->removeConnection(conn, err))
        {
            BMCWEB_LOG_DEBUG << "Remove obmc handler " << handler;

            // Removed last connection so remove the path
            obmcHandlerMap.erase(iter);
        }
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
        removeConnectionAndHandler(conn,
                                   "Failed to call console Connect() method");
        return;
    }

    // Look up the handler
    std::shared_ptr<ObmcHandler> handler = getObmcHandler(conn);
    if (handler == nullptr)
    {
        BMCWEB_LOG_ERROR << "Failed to find the handler";
        removeConnectionAndHandler(conn, "Failed to find the handler");
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

    std::shared_ptr<ObmcHandler> handler = addConnectionAndHandler(conn);
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
        [&conn](const boost::system::error_code& ec,
                const sdbusplus::message::unix_fd& unixfd) {
        connectConsoleSocket(conn, ec, unixfd);
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
        .onclose(removeConnectionAndHandler)
        .onmessage([](crow::websocket::Connection& conn,
                      const std::string& data, [[maybe_unused]] bool isBinary) {
            std::shared_ptr<ObmcHandler> handler = getObmcHandler(conn);
            if (handler != nullptr)
            {
                handler->inputBuffer += data;
                handler->doWrite();
            }
        });
}
} // namespace obmc_console
} // namespace crow
