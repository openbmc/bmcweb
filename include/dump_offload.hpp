#pragma once

#include <signal.h>
#include <sys/select.h>

#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/beast/http.hpp>
#include <boost/process.hpp>
#include <cstdio>
#include <cstdlib>

namespace crow
{
namespace obmc_dump
{

inline void handleDumpOffloadUrl(const crow::Request& req, crow::Response& res,
                                 const std::string& entryId);
inline void resetHandler();

// The max network block device buffer size is 128kb plus 16bytes
// for the message header
static constexpr auto nbdBufferSize = 131088;

/** class Handler
 *  handles data transfer between nbd-client and nbd-server.
 *  This handler invokes nbd-proxy and reads data from socket
 *  and writes on to nbd-client and vice-versa
 */
class Handler : public std::enable_shared_from_this<Handler>
{
  public:
    Handler(const std::string& media, boost::asio::io_context& ios,
            const std::string& entryID) :
        pipeOut(ios),
        pipeIn(ios), media(media), entryID(entryID), doingWrite(false),
        negotiationDone(false), writeonnbd(false),
        outputBuffer(std::make_unique<
                     boost::beast::flat_static_buffer<nbdBufferSize>>()),
        inputBuffer(
            std::make_unique<boost::beast::flat_static_buffer<nbdBufferSize>>())
    {
    }

    ~Handler()
    {
    }

    /**
     * @brief  Invokes InitiateOffload method of dump manager which
     *         directs pldm to start writing on the nbd device.
     *
     * @return void
     */
    void initiateOffloadOnNbdDevice()
    {
        crow::connections::systemBus->async_method_call(
            [this](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
#if BOOST_VERSION >= 107000
                    this->inputBuffer->clear();
                    this->outputBuffer->clear();
#else
                    this->inputBuffer->reset();
                    this->outputBuffer->reset();
#endif
                    resetHandler();

                    return;
                }
            },
            "xyz.openbmc_project.Dump.Manager",
            "/xyz/openbmc_project/dump/entry/" + entryID,
            "xyz.openbmc_project.Dump.Entry", "InitiateOffload");
    }

    /**
     * @brief  Kills nbd-proxy
     *
     * @return void
     */
    void doClose()
    {
        int rc = kill(proxy.id(), SIGTERM);
        if (rc)
        {
            return;
        }
        proxy.wait();
    }

    /**
     * @brief  Starts nbd-proxy
     *
     * @return void
     */
    void connect()
    {
        std::error_code ec;
        proxy = boost::process::child("/usr/sbin/nbd-proxy", media,
                                      boost::process::std_out > pipeOut,
                                      boost::process::std_in < pipeIn, ec);
        if (ec)
        {
            BMCWEB_LOG_ERROR << "Couldn't connect to nbd-proxy: "
                             << ec.message();
            resetHandler();
            return;
        }
        doRead();
    }

    /**
     * @brief  Wait for data on tcp socket from nbd-server.
     *
     * @return void
     */
    void waitForMessageOnSocket()
    {

        std::size_t bytes = inputBuffer->capacity() - inputBuffer->size();

        (*stream).async_read_some(
            inputBuffer->prepare(bytes),
            [this,
             self(shared_from_this())](const boost::system::error_code& ec,
                                       std::size_t bytes_transferred) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG << "Error while reading on socket";
                    doClose();
#if BOOST_VERSION >= 107000
                    this->inputBuffer->clear();
                    this->outputBuffer->clear();
#else
                    this->inputBuffer->reset();
                    this->outputBuffer->reset();
#endif
                    resetHandler();
                    return;
                }

                inputBuffer->commit(bytes_transferred);
                doWrite();
            });
    }

    /**
     * @brief  Writes data on input pipe of nbd-client.
     *
     * @return void
     */
    void doWrite()
    {

        if (doingWrite)
        {
            BMCWEB_LOG_DEBUG << "Already writing.  Bailing out";
            return;
        }

        if (inputBuffer->size() == 0)
        {
            BMCWEB_LOG_DEBUG << "inputBuffer empty.  Bailing out";
            return;
        }

        doingWrite = true;
        boost::asio::async_write(
            pipeIn, inputBuffer->data(),
            [this, self(shared_from_this())](const boost::beast::error_code& ec,
                                             std::size_t bytesWritten) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG << "VM socket port closed";

                    doClose();
#if BOOST_VERSION >= 107000
                    this->inputBuffer->clear();
                    this->outputBuffer->clear();
#else
                    this->inputBuffer->reset();
                    this->outputBuffer->reset();
#endif
                    resetHandler();

                    return;
                }

                doingWrite = false;

                if (negotiationDone == false)
                {
                    // "gDf" is NBD reply magic
                    std::string reply_magic("gDf");
                    std::string reply_string(
                        static_cast<char*>(inputBuffer->data().data()),
                        bytesWritten);
                    std::size_t found = reply_string.find(reply_magic);
                    if (found != std::string::npos)
                    {
                        negotiationDone = true;
                        writeonnbd = true;
                    }
                }

                inputBuffer->consume(bytesWritten);
                waitForMessageOnSocket();
                if (writeonnbd)
                {
                    // NBD Negotiation Complete!!!!. Notify Dump manager to
                    // start dumping the actual data over NBD device
                    initiateOffloadOnNbdDevice();
                    writeonnbd = false;
                }
            });
    }

    /**
     * @brief  Reads data on output pipe of nbd-client and write on
     *         tcp socket.
     *
     * @return void
     */
    void doRead()
    {
        std::size_t bytes = outputBuffer->capacity() - outputBuffer->size();

        pipeOut.async_read_some(
            outputBuffer->prepare(bytes),
            [this](const boost::system::error_code& ec, std::size_t bytesRead) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Couldn't read from VM port: " << ec;
                    return;
                }

                outputBuffer->commit(bytesRead);

                boost::asio::async_write(
                    *stream, outputBuffer->data(),
                    [this](const boost::system::error_code& ec,
                           std::size_t bytes_transferred) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG << "Error while writing on socket";
                            return;
                        }

                        outputBuffer->consume(bytes_transferred);
                        doRead();
                    });
            });
    }

    boost::process::async_pipe pipeOut;
    boost::process::async_pipe pipeIn;
    boost::process::child proxy;
    std::string media;
    std::string entryID;
    bool doingWrite;
    bool negotiationDone;
    bool writeonnbd;
    std::unique_ptr<boost::beast::flat_static_buffer<nbdBufferSize>>
        outputBuffer;
    std::unique_ptr<boost::beast::flat_static_buffer<nbdBufferSize>>
        inputBuffer;
    std::optional<crow::Request::Adaptor> stream;
};

static std::shared_ptr<Handler> handler;
inline void resetHandler()
{

    handler.reset();
}
inline void handleDumpOffloadUrl(const crow::Request& req, crow::Response& res,
                                 const std::string& entryId)
{

    // Run only one instance of Handler, one dump offload can happen at a time
    if (handler != nullptr)
    {
        BMCWEB_LOG_ERROR << "Handler already running";
        res.result(boost::beast::http::status::service_unavailable);
        res.jsonValue["Description"] = "Service is already being used";
        res.end();
        return;
    }

    const char* media = "1";
    boost::asio::io_context* io_con = req.ioService;

    handler = std::make_shared<Handler>(media, *io_con, entryId);
    handler->stream = std::move(req.socket());
    handler->connect();
    handler->waitForMessageOnSocket();
}
} // namespace obmc_dump
} // namespace crow
