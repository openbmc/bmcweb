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
            BMCWEB_LOG_ERROR << "Error in host serial write " << ec;
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
            BMCWEB_LOG_ERROR << "Couldn't read from host serial port: " << ec;
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

inline void connectHandler(const boost::system::error_code& ec)
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

// Make sure that connection user is part of hostconsole group.
// If it is then connect to the socket.
inline void
    checkPermissionAndConnect(crow::websocket::Connection& conn,
                              const boost::system::error_code ec,
                              const dbus::utility::DBusPropertiesMap& userInfo)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR << "GetUserInfo failed...";
        conn.close("Failed to get user information");
        return;
    }

    BMCWEB_LOG_DEBUG << "Check if '" << conn.getUserName()
                     << "' is part of hostconsole group";

    auto userInfoIter =
        std::find_if(userInfo.begin(), userInfo.end(),
                     [](const auto& p) { return p.first == "UserGroups"; });

    if (userInfoIter == userInfo.end())
    {
        BMCWEB_LOG_ERROR << "UserGroups not found...";
        conn.close("Failed to find the user groups");
        return;
    }

    const std::vector<std::string>* userGroups =
        std::get_if<std::vector<std::string>>(&userInfoIter->second);
    if (userGroups == nullptr)
    {
        BMCWEB_LOG_ERROR << "userGroups wasn't a string vector";
        conn.close("Failed to check the user group");
        return;
    }

    // Check if user is part of 'hostconsole' group
    auto userGroupIter = std::find_if(userGroups->begin(), userGroups->end(),
                                      [](const auto& userGroup) {
        return userGroup == "hostconsole";
    });

    if (userGroupIter == userGroups->end())
    {
        BMCWEB_LOG_ERROR << "User is not part of hostconsole group.";
        conn.close("User is not part of hostconsole group");
        return;
    }

    sessions.insert(&conn);
    if (hostSocket == nullptr)
    {
        const std::string consoleName("\0obmc-console", 13);
        boost::asio::local::stream_protocol::endpoint ep(consoleName);

        hostSocket =
            std::make_unique<boost::asio::local::stream_protocol::socket>(
                conn.getIoContext());
        hostSocket->async_connect(ep, connectHandler);
    }
}

inline void onOpen(crow::websocket::Connection& conn)
{
    BMCWEB_LOG_DEBUG << "Connection " << &conn << " opened";

    // Ensure user is part of the hostconsole group
    auto getUserInfoHandler =
        [&conn](const boost::system::error_code& ec,
                const dbus::utility::DBusPropertiesMap& userInfo) {
        checkPermissionAndConnect(conn, ec, userInfo);
    };
    crow::connections::systemBus->async_method_call(
        std::move(getUserInfoHandler), "xyz.openbmc_project.User.Manager",
        "/xyz/openbmc_project/user", "xyz.openbmc_project.User.Manager",
        "GetUserInfo", conn.getUserName());
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

            sessions.erase(&conn);
            if (sessions.empty())
            {
                hostSocket = nullptr;
                inputBuffer.clear();
                inputBuffer.shrink_to_fit();
            }
        })
        .onmessage([]([[maybe_unused]] crow::websocket::Connection& conn,
                      const std::string& data, [[maybe_unused]] bool isBinary) {
            inputBuffer += data;
            doWrite();
        });
}
} // namespace obmc_console
} // namespace crow
