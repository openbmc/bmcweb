#pragma once

#include <app.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/process.hpp>
#include <websocket.hpp>

#include <csignal>

namespace crow
{
namespace obmc_vm
{

static crow::websocket::Connection* session = nullptr;

// The max network block device buffer size is 128kb plus 16bytes
// for the message header:
// https://github.com/NetworkBlockDevice/nbd/blob/master/doc/proto.md#simple-reply-message
static constexpr auto nbdBufferSize = 131088;

class Handler : public std::enable_shared_from_this<Handler>
{
  public:
    Handler(const std::string& mediaIn, boost::asio::io_context& ios) :
        pipeOut(ios), pipeIn(ios), media(mediaIn), doingWrite(false),
        outputBuffer(new boost::beast::flat_static_buffer<nbdBufferSize>),
        inputBuffer(new boost::beast::flat_static_buffer<nbdBufferSize>)
    {}

    ~Handler() = default;

    void doClose()
    {
        // boost::process::child::terminate uses SIGKILL, need to send SIGTERM
        // to allow the proxy to stop nbd-client and the USB device gadget.
        int rc = kill(proxy.id(), SIGTERM);
        if (rc)
        {
            return;
        }
        proxy.wait();
    }

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
            if (session != nullptr)
            {
                session->close("Error connecting to nbd-proxy");
            }
            return;
        }
        doWrite();
        doRead();
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
        pipeIn.async_write_some(
            inputBuffer->data(),
            [this, self(shared_from_this())](boost::beast::error_code ec,
                                             std::size_t bytesWritten) {
                BMCWEB_LOG_DEBUG << "Wrote " << bytesWritten << "bytes";
                doingWrite = false;
                inputBuffer->consume(bytesWritten);

                if (session == nullptr)
                {
                    return;
                }
                if (ec == boost::asio::error::eof)
                {
                    session->close("VM socket port closed");
                    return;
                }
                if (ec)
                {
                    session->close("Error in writing to proxy port");
                    BMCWEB_LOG_ERROR << "Error in VM socket write " << ec;
                    return;
                }
                doWrite();
            });
    }

    void doRead()
    {
        std::size_t bytes = outputBuffer->capacity() - outputBuffer->size();

        pipeOut.async_read_some(
            outputBuffer->prepare(bytes),
            [this, self(shared_from_this())](
                const boost::system::error_code& ec, std::size_t bytesRead) {
                BMCWEB_LOG_DEBUG << "Read done.  Read " << bytesRead
                                 << " bytes";
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Couldn't read from VM port: " << ec;
                    if (session != nullptr)
                    {
                        session->close("Error in connecting to VM port");
                    }
                    return;
                }
                if (session == nullptr)
                {
                    return;
                }

                outputBuffer->commit(bytesRead);
                std::string_view payload(
                    static_cast<const char*>(outputBuffer->data().data()),
                    bytesRead);
                session->sendBinary(payload);
                outputBuffer->consume(bytesRead);

                doRead();
            });
    }

    boost::process::async_pipe pipeOut;
    boost::process::async_pipe pipeIn;
    boost::process::child proxy;
    std::string media;
    bool doingWrite;

    std::unique_ptr<boost::beast::flat_static_buffer<nbdBufferSize>>
        outputBuffer;
    std::unique_ptr<boost::beast::flat_static_buffer<nbdBufferSize>>
        inputBuffer;
};

static std::shared_ptr<Handler> handler;

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/vm/0/0")
        .privileges({"ConfigureComponents", "ConfigureManager"})
        .websocket()
        .onopen([](crow::websocket::Connection& conn,
                   const std::shared_ptr<bmcweb::AsyncResp>&) {
            BMCWEB_LOG_DEBUG << "Connection " << &conn << " opened";

            if (session != nullptr)
            {
                conn.close("Session already connected");
                return;
            }

            if (handler != nullptr)
            {
                conn.close("Handler already running");
                return;
            }

            session = &conn;

            // media is the last digit of the endpoint /vm/0/0. A future
            // enhancement can include supporting different endpoint values.
            const char* media = "0";
            handler = std::make_shared<Handler>(media, conn.getIoContext());
            handler->connect();
        })
        .onclose([](crow::websocket::Connection& conn,
                    const std::string& /*reason*/) {
            if (&conn != session)
            {
                return;
            }

            session = nullptr;
            handler->doClose();
            handler->inputBuffer->clear();
            handler->outputBuffer->clear();
            handler.reset();
        })
        .onmessage([](crow::websocket::Connection& conn,
                      const std::string& data, bool) {
            if (data.length() > handler->inputBuffer->capacity())
            {
                BMCWEB_LOG_ERROR << "Buffer overrun when writing "
                                 << data.length() << " bytes";
                conn.close("Buffer overrun");
                return;
            }

            boost::asio::buffer_copy(handler->inputBuffer->prepare(data.size()),
                                     boost::asio::buffer(data));
            handler->inputBuffer->commit(data.size());
            handler->doWrite();
        });
}

} // namespace obmc_vm
} // namespace crow
