#pragma once

#include "app.hpp"
#include "logging.hpp"
#include "websocket.hpp"

#include <pty.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>

#include <boost/asio/buffer.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/process/io.hpp>
#include <boost/system/error_code.hpp>

#include <array>
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <variant>

namespace crow
{

namespace obmc_shell
{

class Handler : public std::enable_shared_from_this<Handler>
{
  public:
    explicit Handler(crow::websocket::Connection* conn) :
        session(conn), streamFileDescriptor(conn->getIoContext())
    {
        outputBuffer.fill(0);
    }

    ~Handler() = default;

    Handler(const Handler&) = delete;
    Handler(Handler&&) = delete;
    Handler& operator=(const Handler&) = delete;
    Handler& operator=(Handler&&) = delete;

    void doClose()
    {
        streamFileDescriptor.close();

        // boost::process::child::terminate uses SIGKILL, need to send SIGTERM
        int rc = kill(pid, SIGKILL);
        session = nullptr;
        if (rc != 0)
        {
            return;
        }
        waitpid(pid, nullptr, 0);
    }

    void connect()
    {
        pid = forkpty(&ttyFileDescriptor, nullptr, nullptr, nullptr);

        if (pid == -1)
        {
            // ERROR
            if (session != nullptr)
            {
                session->close("Error creating child process for login shell.");
            }
            return;
        }
        if (pid == 0)
        {
            // CHILD

            auto userName = session->getUserName();
            if (!userName.empty())
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
                execl("/bin/login", "/bin/login", "-f", userName.c_str(), NULL);
                // execl only returns on fail
                BMCWEB_LOG_ERROR("execl() for /bin/login failed: {}", errno);
                session->close("Internal Error Login failed");
            }
            else
            {
                session->close("Error session user name not found");
            }
            return;
        }
        if (pid > 0)
        {
            // PARENT

            // for io operation assing file discriptor
            // to boost stream_descriptor
            streamFileDescriptor.assign(ttyFileDescriptor);
            doWrite();
            doRead();
        }
    }

    void doWrite()
    {
        if (session == nullptr)
        {
            BMCWEB_LOG_DEBUG("session is closed");
            return;
        }
        if (doingWrite)
        {
            BMCWEB_LOG_DEBUG("Already writing.  Bailing out");
            return;
        }

        if (inputBuffer.empty())
        {
            BMCWEB_LOG_DEBUG("inputBuffer empty.  Bailing out");
            return;
        }

        doingWrite = true;
        streamFileDescriptor.async_write_some(
            boost::asio::buffer(inputBuffer.data(), inputBuffer.size()),
            [this, self(shared_from_this())](
                const boost::system::error_code& ec, std::size_t bytesWritten) {
            BMCWEB_LOG_DEBUG("Wrote {} bytes", bytesWritten);
            doingWrite = false;
            inputBuffer.erase(0, bytesWritten);
            if (ec == boost::asio::error::eof)
            {
                session->close("ssh socket port closed");
                return;
            }
            if (ec)
            {
                session->close("Error in writing to processSSH port");
                BMCWEB_LOG_ERROR("Error in ssh socket write {}", ec);
                return;
            }
            doWrite();
        });
    }

    void doRead()
    {
        if (session == nullptr)
        {
            BMCWEB_LOG_DEBUG("session is closed");
            return;
        }
        streamFileDescriptor.async_read_some(
            boost::asio::buffer(outputBuffer.data(), outputBuffer.size()),
            [this, self(shared_from_this())](
                const boost::system::error_code& ec, std::size_t bytesRead) {
            BMCWEB_LOG_DEBUG("Read done.  Read {} bytes", bytesRead);
            if (session == nullptr)
            {
                BMCWEB_LOG_DEBUG("session is closed");
                return;
            }
            if (ec)
            {
                BMCWEB_LOG_ERROR("Couldn't read from ssh port: {}", ec);
                session->close("Error in connecting to ssh port");
                return;
            }
            std::string_view payload(outputBuffer.data(), bytesRead);
            session->sendBinary(payload);
            doRead();
        });
    }

    // this has to public
    std::string inputBuffer;

  private:
    crow::websocket::Connection* session;
    boost::asio::posix::stream_descriptor streamFileDescriptor;
    bool doingWrite{false};
    int ttyFileDescriptor{0};
    pid_t pid{0};

    std::array<char, 4096> outputBuffer{};
};

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
static std::map<crow::websocket::Connection*, std::shared_ptr<Handler>>
    mapHandler;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

inline void onOpen(crow::websocket::Connection& conn)
{
    BMCWEB_LOG_DEBUG("Connection {} opened", logPtr(&conn));
    auto it = mapHandler.find(&conn);
    if (it == mapHandler.end())
    {
        auto insertData = mapHandler.emplace(&conn,
                                             std::make_shared<Handler>(&conn));
        if (std::get<bool>(insertData))
        {
            std::get<0>(insertData)->second->connect();
        }
    }
}

inline void onClose(crow::websocket::Connection& conn,
                    const std::string& reason)
{
    BMCWEB_LOG_DEBUG("bmc-shell console.onclose(reason = '{}')", reason);
    auto it = mapHandler.find(&conn);
    if (it != mapHandler.end())
    {
        it->second->doClose();
        mapHandler.erase(it);
    }
}

inline void onMessage(crow::websocket::Connection& conn,
                      const std::string& data, [[maybe_unused]] bool isBinary)
{
    auto it = mapHandler.find(&conn);
    if (it != mapHandler.end())
    {
        it->second->inputBuffer += data;
        it->second->doWrite();
    }
    else
    {
        BMCWEB_LOG_ERROR("connection to socket not found");
    }
}

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/bmc-console")
        .privileges({{"OemIBMPerformService"}})
        .websocket()
        .onopen(onOpen)
        .onclose(onClose)
        .onmessage(onMessage);
}

} // namespace obmc_shell
} // namespace crow
