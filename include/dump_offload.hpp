#pragma once

#include <sys/select.h>

#include <boost/asio.hpp>
#include <boost/asio/basic_socket_acceptor.hpp>
#include <boost/asio/basic_stream_socket.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/beast/http.hpp>
#include <http_stream.hpp>

namespace crow
{
namespace obmc_dump
{

using boost::asio::local::stream_protocol;

std::string unixSocketPathDir = "/var/lib/bmcweb/";

inline void handleDumpOffloadUrl(const crow::Request& req, crow::Response& res,
                                 const std::string& entryId,
                                 const std::string& dumpEntryType);
inline void resetHandler();

static constexpr auto socketBufferSize = 1024 * 1024;

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
        dumpType(dumpTypeIn),
        outputBuffer(std::make_unique<
                     boost::beast::flat_static_buffer<socketBufferSize>>()),
        unixSocketPath(unixSocketPathIn),
        acceptor(ios, stream_protocol::endpoint(unixSocketPath)),
        peerSocket(ios), dumpSize(0)
    {}

    /**
     * @brief  Accepts unix domain socket connection and
     *         starts reading data from peer unix domain socket.
     *
     * @return void
     */

    void acceptSocketConnection()
    {
        acceptor.async_accept([this, self(shared_from_this())](
                                  boost::system::error_code ec,
                                  stream_protocol::socket socket) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "UNIX Socket: async_accept error "
                                 << ec.message();
                this->acceptor.close();
                this->connection->streamres.result(
                    boost::beast::http::status::internal_server_error);
                this->connection->close();
                return;
            }

            this->peerSocket = std::move(socket);
            this->connection->sendStreamHeaders(std::to_string(this->dumpSize));
            this->doReadStream();
        });
    }

    /**
     * @brief  Invokes InitiateOffload method of dump manager which
     *         directs dump manager to start writing on unix domain socket.
     *
     * @return void
     */
    void initiateOffloadOnNbdDevice()
    {
        crow::connections::systemBus->async_method_call(
            [this,
             self(shared_from_this())](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                    this->acceptor.close();
                    this->connection->sendStreamErrorStatus(
                        boost::beast::http::status::internal_server_error);
                    this->connection->close();
                    return;
                }
            },
            "xyz.openbmc_project.Dump.Manager",
            "/xyz/openbmc_project/dump/" + dumpType + "/entry/" + entryID,
            "xyz.openbmc_project.Dump.Entry", "InitiateOffload",
            unixSocketPath);
    }

    void getDumpSize(const std::string& entryID, const std::string& dumpType)
    {
        crow::connections::systemBus->async_method_call(
            [this,
             self(shared_from_this())](const boost::system::error_code ec,
                                       const std::variant<uint64_t>& size) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR
                        << "DBUS response error: Unable to get the dump size "
                        << ec;
                    this->acceptor.close();
                    this->connection->sendStreamErrorStatus(
                        boost::beast::http::status::internal_server_error);
                    this->connection->close();
                    return;
                }
                const uint64_t* dumpsize = std::get_if<uint64_t>(&size);
                this->dumpSize = *dumpsize;
                this->initiateOffloadOnNbdDevice();
            },
            "xyz.openbmc_project.Dump.Manager",
            "/xyz/openbmc_project/dump/" + dumpType + "/entry/" + entryID,
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
        std::size_t bytes = outputBuffer->capacity() - outputBuffer->size();

        peerSocket.async_read_some(
            outputBuffer->prepare(bytes),
            [this, self(shared_from_this())](
                const boost::system::error_code& ec, std::size_t bytesRead) {
                if (ec)
                {
                    if (ec != boost::asio::error::eof)
                    {
                        BMCWEB_LOG_ERROR << "Couldn't read from local peer: "
                                         << ec;
                        this->connection->sendStreamErrorStatus(
                            boost::beast::http::status::internal_server_error);
                    }
                    this->acceptor.close();
                    this->connection->close();
                    return;
                }

                outputBuffer->commit(bytesRead);
                std::string_view payload(
                    static_cast<const char*>(outputBuffer->data().data()),
                    bytesRead);

                auto streamHandler = [this, bytesRead,
                                      self(shared_from_this())]() {
                    this->outputBuffer->consume(bytesRead);
                    this->doReadStream();
                };
                this->connection->sendMessage(payload, streamHandler);
            });
    }

    /**
     * @brief  Resets output buffers.
     * @return void
     */
    void resetBuffers()
    {
        this->outputBuffer->clear();
    }

    std::string entryID;
    std::string dumpType;
    std::unique_ptr<boost::beast::flat_static_buffer<socketBufferSize>>
        outputBuffer;
    std::string unixSocketPath;
    stream_protocol::acceptor acceptor;
    stream_protocol::socket peerSocket;
    uint64_t dumpSize;
    crow::streamsocket::Connection* connection = nullptr;
};

static boost::container::flat_map<crow::streamsocket::Connection*,
                                  std::shared_ptr<Handler>>
    handlers;

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Managers/bmc/LogServices/Dump/attachment/<str>")
        .privileges({"ConfigureComponents", "ConfigureManager"})
        .streamsocket()
        .onopen([](crow::streamsocket::Connection& conn) {
            std::string url(conn.req.target());
            std::size_t pos = url.rfind('/');
            std::string dumpId;
            if (pos != std::string::npos)
            {
                dumpId = url.substr(pos + 1);
            }

            std::string dumpType = "bmc";
            boost::asio::io_context* ioCon = conn.getIoContext();

            std::string unixSocketPath =
                unixSocketPathDir + dumpType + "_dump_" + dumpId;

            ::unlink(unixSocketPath.c_str());
            handlers[&conn] = std::make_shared<Handler>(
                *ioCon, dumpId, dumpType, unixSocketPath);
            handlers[&conn]->connection = &conn;
            handlers[&conn]->getDumpSize(dumpId, dumpType);
            handlers[&conn]->acceptSocketConnection();
        })
        .onclose([](crow::streamsocket::Connection& conn) {
            auto handler = handlers.find(&conn);
            if (handler == handlers.end())
            {
                BMCWEB_LOG_DEBUG << "No handler to cleanup";
                return;
            }
            ::unlink(handler->second->unixSocketPath.c_str());
            std::remove(handler->second->unixSocketPath.c_str());
            handler->second->outputBuffer->clear();
            handlers.erase(handler);
        });

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/system/LogServices/Dump/attachment/<str>")
        .privileges({"ConfigureComponents", "ConfigureManager"})
        .streamsocket()
        .onopen([](crow::streamsocket::Connection& conn) {
            std::string url(conn.req.target());
            std::size_t pos = url.rfind('/');
            std::string dumpId;
            if (pos != std::string::npos)
            {
                dumpId = url.substr(pos + 1);
            }

            std::string dumpType = "system";
            boost::asio::io_context* ioCon = conn.getIoContext();

            std::string unixSocketPath =
                unixSocketPathDir + dumpType + "_dump_" + dumpId;

            ::unlink(unixSocketPath.c_str());

            handlers[&conn] = std::make_shared<Handler>(
                *ioCon, dumpId, dumpType, unixSocketPath);
            handlers[&conn]->connection = &conn;
            handlers[&conn]->getDumpSize(dumpId, dumpType);
            handlers[&conn]->acceptSocketConnection();
        })
        .onclose([](crow::streamsocket::Connection& conn) {
            auto handler = handlers.find(&conn);
            if (handler == handlers.end())
            {
                BMCWEB_LOG_DEBUG << "No handler to cleanup";
                return;
            }
            ::unlink(handler->second->unixSocketPath.c_str());
            std::remove(handler->second->unixSocketPath.c_str());

            handlers.erase(handler);
            handler->second->outputBuffer->clear();
        });
}

} // namespace obmc_dump
} // namespace crow
