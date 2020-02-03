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

static constexpr auto nbdBufferSize = 131088;

class Handler : public std::enable_shared_from_this<Handler>
{
    using response_type =
        boost::beast::http::response<boost::beast::http::string_body>;

  public:
    Handler(const std::string& media, boost::asio::io_context& ios,
            const std::string& entryID) :
        pipeOut(ios),
        pipeIn(ios), media(media), entryID(entryID), doingWrite(false),
        negotiation_done(false), writeonnbd(false),
        outputBuffer(new boost::beast::flat_static_buffer<nbdBufferSize>),
        inputBuffer(new boost::beast::flat_static_buffer<nbdBufferSize>)
    {
    }

    ~Handler()
    {
    }

    void writeto_nbd_device()
    {
        crow::connections::systemBus->async_method_call(
            [](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                    return;
                }
            },
            "xyz.openbmc_project.Dump.Manager",
            "/xyz/openbmc_project/dump/entry/" + entryID,
            "xyz.openbmc_project.Dump.Entry", "InitiateOffload");
    }
    void doClose()
    {
        int rc = kill(proxy.id(), SIGTERM);
        if (rc)
        {
            return;
        }
        proxy.wait();
    }

    void connect(const crow::Request& req)
    {
        std::error_code ec;
        proxy = boost::process::child("/usr/sbin/nbd-proxy", media,
                                      boost::process::std_out > pipeOut,
                                      boost::process::std_in < pipeIn, ec);
        if (ec)
        {
            BMCWEB_LOG_ERROR << "Couldn't connect to nbd-proxy: "
                             << ec.message();
            return;
        }
        doRead();
    }

    void waitForMessageOnSocket()
    {

        std::size_t bytes = inputBuffer->capacity() - inputBuffer->size();

        (*stream).async_read_some(
            inputBuffer->prepare(bytes),
            [&](const boost::system::error_code& ec,
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
            [this, self(shared_from_this())](boost::beast::error_code ec,
                                             std::size_t bytesWritten) {
                doingWrite = false;

                if (negotiation_done == false)
                {
                    char* buf = static_cast<char*>(
                        malloc((sizeof(char) * bytesWritten)));
                    memset(buf, 0, bytesWritten);
                    boost::asio::buffer_copy(
                        boost::asio::buffer(buf, bytesWritten),
                        inputBuffer->data());
                    std::string reply_magic("gDf");
                    std::string reply_string(buf, bytesWritten);
                    std::size_t found = reply_string.find(reply_magic);
                    if (found != std::string::npos)
                    {
                        negotiation_done = true;
                        writeonnbd = true;
                    }
                    free(buf);
                }

                inputBuffer->consume(bytesWritten);

                if (ec == boost::asio::error::eof)
                {
                    doClose();
                    return;
                }

                if (ec)
                {
                    BMCWEB_LOG_DEBUG << "VM socket port closed";

                    doClose();
                    return;
                }
                waitForMessageOnSocket();
                if (writeonnbd)
                {
                    writeto_nbd_device();
                    writeonnbd = false;
                }
            });
    }
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
    bool negotiation_done;
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

    if (handler != nullptr)
    {
        BMCWEB_LOG_DEBUG << "Handler already running";
        return;
    }

    const char* media = "1";
    boost::asio::io_context* io_con = req.ioService;

    handler = std::make_shared<Handler>(media, *io_con, entryId);
    handler->stream = std::move(const_cast<crow::Request&>(req).socket());
    handler->connect(req);
    handler->waitForMessageOnSocket();
}
} // namespace obmc_dump
} // namespace crow
