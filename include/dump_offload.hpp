#pragma once

#include <sys/select.h>

#include <boost/asio.hpp>
#include <boost/asio/basic_socket_acceptor.hpp>
#include <boost/asio/basic_stream_socket.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/beast/http.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/system/error_code.hpp>
#include <http_stream.hpp>
#include <ibm/utils.hpp>

#include <cstddef>
#include <filesystem>
#include <random>
#include <string>

namespace crow
{
namespace obmc_dump
{

static constexpr std::string_view unixSocketPathDir =
    "/tmp/DumpOffloadSockets/";

inline void handleDumpOffloadUrl(const crow::Request& req, crow::Response& res,
                                 const std::string& entryId,
                                 const std::string& dumpEntryType);
inline void resetHandler();

static constexpr size_t socketBufferSize = static_cast<size_t>(64 * 1024);
static constexpr uint8_t maxConnectRetryCount = 3;

/** class Handler
 *  Handles data transfer between unix domain socket and http connection socket.
 *  This handler invokes dump offload reads data from unix domain socket
 *  and writes on to http stream connection socket.
 */
class Handler : public std::enable_shared_from_this<Handler>
{
  public:
    Handler(boost::asio::io_context& ios, const std::string& entryIDIn,
            const std::string& dumpTypeIn,
            const std::string& unixSocketPathIn) :
        entryID(entryIDIn),
        dumpType(dumpTypeIn), unixSocketPath(unixSocketPathIn), unixSocket(ios),
        waitTimer(ios)
    {}

    /**
     * @brief Connects to unix socket to read dump data
     *
     * @return void
     */
    void doConnect()
    {
        this->unixSocket.async_connect(
            unixSocketPath.c_str(), [this, self(shared_from_this())](
                                        const boost::system::error_code& ec) {
            if (ec)
            {
                // TODO:
                // right now we don't have dbus method which can make sure
                // unix socket is ready to accept connection so its possible
                // that bmcweb can try to connect to socket before even
                // socket setup, so using retry mechanism with timeout.
                if (ec == boost::system::errc::no_such_file_or_directory ||
                    ec == boost::system::errc::connection_refused)
                {
                    BMCWEB_LOG_DEBUG("UNIX Socket: async_connect {}", ec);
                    retrySocketConnect();
                    return;
                }
                BMCWEB_LOG_ERROR("UNIX Socket: async_connect error {}", ec);
                waitTimer.cancel();
                this->connection->sendStreamErrorStatus(
                    boost::beast::http::status::internal_server_error);
                this->connection->close();
                this->cleanupSocketFiles();
                return;
            }
            waitTimer.cancel();
            this->connection->sendStreamHeaders(std::to_string(this->dumpSize),
                                                "application/octet-stream");
            this->doReadStream();
        });
    }

    /**
     * @brief  Invokes InitiateOffload method of dump manager which
     *         directs dump manager to start writing on unix domain socket.
     *
     * @return void
     */
    void initiateOffload()
    {
        crow::connections::systemBus->async_method_call(
            [this,
             self(shared_from_this())](const boost::system::error_code& ec) {
            if (ec)
            {
                if (ec.value() == EBADR)
                {
                    this->connection->sendStreamErrorStatus(
                        boost::beast::http::status::not_found);
                }
                else
                {
                    BMCWEB_LOG_ERROR("DBUS response error: {}", ec);
                    this->connection->sendStreamErrorStatus(
                        boost::beast::http::status::internal_server_error);
                }
                this->connection->close();
                this->cleanupSocketFiles();
                return;
            }
        },
            "xyz.openbmc_project.Dump.Manager",
            "/xyz/openbmc_project/dump/" + dumpType + "/entry/" + entryID,
            "xyz.openbmc_project.Dump.Entry", "InitiateOffload",
            unixSocketPath.c_str());
    }

    /**
     * @brief  This function setup a timer for retrying unix socket connect.
     *
     * @return void
     */
    void retrySocketConnect()
    {
        waitTimer.expires_after(std::chrono::milliseconds(1000));

        waitTimer.async_wait([this, self(shared_from_this())](
                                 const boost::system::error_code& ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("Async_wait failed {}", ec);
                return;
            }

            if (connectRetryCount < maxConnectRetryCount)
            {
                BMCWEB_LOG_ERROR("Failed to connect, reached max retry count: ",
                                 connectRetryCount);
                connectRetryCount++;
                doConnect();
            }
            else
            {
                BMCWEB_LOG_ERROR("Failed to connect, reached max retry count: ",
                                 connectRetryCount);
                waitTimer.cancel();
                this->cleanupSocketFiles();
                this->connection->setStreamHeaders("Retry-After", "60");
                this->connection->sendStreamErrorStatus(
                    boost::beast::http::status::service_unavailable);
                this->connection->close();
                return;
            }
        });
    }

    void resetOffloadURI()
    {
        std::string value;
        crow::connections::systemBus->async_method_call(
            [this,
             self(shared_from_this())](const boost::system::error_code& ec) {
            if (ec)
            {
                if (ec.value() == EBADR)
                {
                    this->connection->sendStreamErrorStatus(
                        boost::beast::http::status::not_found);
                }
                else
                {
                    BMCWEB_LOG_ERROR("DBUS response error: Unable to set "
                                     "the dump OffloadUri {}",
                                     ec);
                    this->connection->sendStreamErrorStatus(
                        boost::beast::http::status::internal_server_error);
                }
                return;
            }
            BMCWEB_LOG_CRITICAL("INFO: Reset OffloadUri of {} dump id {}",
                                dumpType, entryID);
        },
            "xyz.openbmc_project.Dump.Manager",
            "/xyz/openbmc_project/dump/" + dumpType + "/entry/" + entryID,
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Dump.Entry", "OffloadUri",
            std::variant<std::string>(value));
    }

    void cleanupSocketFiles()
    {
        std::error_code ec;
        bool fileExists = std::filesystem::exists(unixSocketPath, ec);
        if (ec)
        {
            this->connection->sendStreamErrorStatus(
                boost::beast::http::status::internal_server_error);
            return;
        }
        if (fileExists)
        {
            unixSocket.close();
            std::remove(unixSocketPath.c_str());
        }
    }

    void getDumpSize(const std::string& dumpEntryID,
                     const std::string& dumpEntryType)
    {
        crow::connections::systemBus->async_method_call(
            [this,
             self(shared_from_this())](const boost::system::error_code& ec,
                                       const std::variant<uint64_t>& size) {
            if (ec)
            {
                if (ec.value() == EBADR)
                {
                    this->connection->sendStreamErrorStatus(
                        boost::beast::http::status::not_found);
                }
                else
                {
                    BMCWEB_LOG_ERROR(
                        "DBUS response error: Unable to get the dump size {}",
                        ec);
                    this->connection->sendStreamErrorStatus(
                        boost::beast::http::status::internal_server_error);
                }
                this->connection->close();
                this->cleanupSocketFiles();
                return;
            }
            const uint64_t* dumpsize = std::get_if<uint64_t>(&size);
            if (dumpsize == nullptr)
            {
                BMCWEB_LOG_ERROR("DBUS response error: Unable to get "
                                 "the dump size value {}",
                                 ec);
                this->connection->sendStreamErrorStatus(
                    boost::beast::http::status::internal_server_error);
                this->connection->close();
                this->cleanupSocketFiles();
                return;
            }
            this->dumpSize = *dumpsize;
            this->initiateOffload();
            this->doConnect();
        },
            "xyz.openbmc_project.Dump.Manager",
            "/xyz/openbmc_project/dump/" + dumpEntryType + "/entry/" +
                dumpEntryID,
            "org.freedesktop.DBus.Properties", "Get",
            "xyz.openbmc_project.Dump.Entry", "Size");
    }

    /**
     * @brief  Reads data from unix domain socket and writes on
     *         http stream connection socket.
     *
     * @return void
     */

    void doReadStream()
    {
        std::size_t bytes = this->outputBuffer.capacity() -
                            this->outputBuffer.size();

        this->unixSocket.async_read_some(
            this->outputBuffer.prepare(bytes),
            [this, self(shared_from_this())](
                const boost::system::error_code& ec, std::size_t bytesRead) {
            if (ec)
            {
                if (ec != boost::asio::error::eof)
                {
                    BMCWEB_LOG_ERROR("Couldn't read from local peer: {}", ec);
                    this->connection->sendStreamErrorStatus(
                        boost::beast::http::status::internal_server_error);
                    this->connection->close();
                    return;
                }
                BMCWEB_LOG_CRITICAL("INFO: Hit Dump end of file");
                this->connection->completionStatus = true;
                this->connection->close();
                return;
            }

            this->outputBuffer.commit(bytesRead);
            auto streamHandler = [this, bytesRead, self(shared_from_this())]() {
                this->outputBuffer.consume(bytesRead);
                this->doReadStream();
            };
            this->connection->sendMessage(this->outputBuffer.data(),
                                          streamHandler);
        });
    }

    std::string entryID;
    std::string dumpType;
    boost::beast::flat_static_buffer<socketBufferSize> outputBuffer;
    std::filesystem::path unixSocketPath;
    boost::asio::local::stream_protocol::socket unixSocket;
    uint64_t dumpSize{0};
    boost::asio::steady_timer waitTimer;
    crow::streaming_response::Connection* connection = nullptr;
    uint16_t connectRetryCount{0};
};

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
static boost::container::flat_map<crow::streaming_response::Connection*,
                                  std::shared_ptr<Handler>>
    systemHandlers;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

inline void resetHandlers()
{
    if (!systemHandlers.empty())
    {
        auto handler = systemHandlers.begin();
        if (handler == systemHandlers.end())
        {
            BMCWEB_LOG_DEBUG("No handler to cleanup");
            return;
        }
        if ((handler->second->dumpType == "system") ||
            (handler->second->dumpType == "resource"))
        {
            handler->first->close();
            BMCWEB_LOG_CRITICAL("INFO: {} dump resetHandlers cleanup",
                                handler->second->dumpType);
        }
    }
}

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/system/LogServices/Dump/Entries/<str>/attachment/")
        .privileges({{"ConfigureComponents", "ConfigureManager"}})
        .streamingResponse()
        .onopen([](crow::streaming_response::Connection& conn) {
        if (!systemHandlers.empty())
        {
            BMCWEB_LOG_WARNING("Can't allow dump offload opertaion, one "
                               "of the host dump is already offloading");
            conn.sendStreamErrorStatus(
                boost::beast::http::status::service_unavailable);
            conn.close();
            return;
        }

        std::string url(conn.req.target());
        std::string startDelimiter = "Entries/";
        std::size_t pos1 = url.rfind(startDelimiter);
        std::size_t pos2 = url.rfind("/attachment");
        if (pos1 == std::string::npos || pos2 == std::string::npos)
        {
            BMCWEB_LOG_WARNING("Unable to extract the dump id");
            conn.sendStreamErrorStatus(boost::beast::http::status::not_found);
            conn.close();
            return;
        }
        std::string dumpEntry =
            url.substr(pos1 + startDelimiter.length(),
                       pos2 - pos1 - startDelimiter.length());

        // System and Resource dump entries are currently being
        // listed under /Systems/system/LogServices/Dump/Entries/
        // redfish path. To differentiate between the two, the dump
        // entries would be listed as System_<id> and Resource_<id> for
        // the respective dumps. Hence the dump id and type are being
        // extracted here from the above format.
        std::string dumpId;
        std::string dumpType;
        std::size_t idPos = dumpEntry.rfind('_');

        if (idPos != std::string::npos)
        {
            dumpType =
                boost::algorithm::to_lower_copy(dumpEntry.substr(0, idPos));
            dumpId = dumpEntry.substr(idPos + 1);
        }

        boost::asio::io_context* ioCon = conn.getIoContext();

        // Generating random id to create unique socket file
        // for each dump offload request
        std::random_device rd;
        std::default_random_engine gen(rd());
        std::uniform_int_distribution<> dist{0, 1024};
        std::string unixSocketPath = std::string(unixSocketPathDir) + dumpType +
                                     "_dump_" + std::to_string(dist(gen));
        systemHandlers[&conn] =
            std::make_shared<Handler>(*ioCon, dumpId, dumpType, unixSocketPath);
        systemHandlers[&conn]->connection = &conn;

        if (!crow::ibm_utils::createDirectory(unixSocketPathDir))
        {
            systemHandlers[&conn]->connection->sendStreamErrorStatus(
                boost::beast::http::status::not_found);
            systemHandlers[&conn]->connection->close();
            return;
        }
        BMCWEB_LOG_CRITICAL("INFO: {}  dump id {} offload initiated by: ",
                            dumpType, dumpId, conn.req.session->clientIp);
        systemHandlers[&conn]->getDumpSize(dumpId, dumpType);
    }).onclose([](crow::streaming_response::Connection& conn, bool& status) {
        auto handler = systemHandlers.find(&conn);
        if (handler == systemHandlers.end())
        {
            BMCWEB_LOG_DEBUG("No handler to cleanup");
            return;
        }
        handler->second->cleanupSocketFiles();
        if (!status)
        {
            handler->second->resetOffloadURI();
        }
        handler->second->outputBuffer.clear();
        systemHandlers.clear();
    });
}

} // namespace obmc_dump
} // namespace crow
