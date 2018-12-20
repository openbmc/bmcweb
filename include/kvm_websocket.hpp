#pragma once
#include <crow/app.h>
#include <crow/websocket.h>
#include <sys/socket.h>

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <webserver_common.hpp>

namespace crow
{
namespace obmc_kvm 
{

static std::unique_ptr<boost::asio::ip::tcp::socket> hostSocket;

// TODO(ed) validate that these buffer sizes are sane
static boost::beast::flat_static_buffer<1024U * 50U> outputBuffer;
static boost::beast::flat_static_buffer<1024U> inputBuffer;

static crow::websocket::Connection* session = nullptr;

static bool doingWrite = false;

inline void doWrite()
{
    if (doingWrite)
    {
        BMCWEB_LOG_DEBUG << "Already writing.  Bailing out";
        return;
    }
    if (inputBuffer.size() == 0)
    {
        BMCWEB_LOG_DEBUG << "inputBuffer empty.  Bailing out";
        return;
    }

    doingWrite = true;
    hostSocket->async_write_some(
        inputBuffer.data(),
        [](boost::beast::error_code ec, std::size_t bytes_written) {
            BMCWEB_LOG_DEBUG << "Wrote " << bytes_written << "bytes";
            doingWrite = false;
            inputBuffer.consume(bytes_written);

            if (session == nullptr)
            {
                return;
            }
            if (ec == boost::asio::error::eof)
            {
                session->close("KVM socket port closed");
                return;
            }
            if (ec)
            {
                session->close("Error in reading to host port");
                BMCWEB_LOG_ERROR << "Error in KVM socket write " << ec;
                return;
            }
            doWrite();
        });
}

inline void doRead();

inline void readDone(const boost::system::error_code& ec, std::size_t bytesRead)
{
    outputBuffer.commit(bytesRead);
    BMCWEB_LOG_DEBUG << "read done.  Read " << bytesRead << " bytes";
    if (ec)
    {
        BMCWEB_LOG_ERROR << "Couldn't read from KVM socket port: " << ec;
        if (session != nullptr)
        {
            session->close("Error in connecting to KVM port");
        }
        return;
    }
    if (session == nullptr)
    {
        return;
    }

    auto buffer = outputBuffer.data();
    boost::beast::string_view payload(static_cast<const char*>(buffer.data()),
                                      bytesRead);
    BMCWEB_LOG_DEBUG << "Sending payload size " << payload.size();
    session->sendBinary(payload);
    outputBuffer.consume(bytesRead);

    doRead();
}

inline void doRead()
{
    std::size_t bytes = outputBuffer.capacity() - outputBuffer.size();
    BMCWEB_LOG_DEBUG << "Reading " << bytes << " from kvm socket";
    hostSocket->async_read_some(
        outputBuffer.prepare(outputBuffer.capacity() - outputBuffer.size()),
        readDone);
}

inline void connectHandler(const boost::system::error_code& ec)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR << "Couldn't connect to KVM socket port: " << ec;
        if (session != nullptr)
        {
            session->close("Error in connecting to KVM port");
        }
        return;
    }

    doWrite();
    doRead();
}

inline void requestRoutes(CrowApp& app)
{
    BMCWEB_ROUTE(app, "/kvm/0")
        .websocket()
        .onopen([](crow::websocket::Connection& conn) {
            BMCWEB_LOG_DEBUG << "Connection " << &conn << " opened";

            if (session != nullptr)
            {
                conn.close("User already connected");
                return;
            }

            session = &conn;
            if (hostSocket == nullptr)
            {
                boost::asio::ip::tcp::endpoint endpoint(
                    boost::asio::ip::address::from_string("127.0.0.1"), 5900);

                hostSocket = std::make_unique<boost::asio::ip::tcp::socket>(
                    conn.getIoService());
                hostSocket->async_connect(endpoint, connectHandler);
            }
        })
        .onclose(
            [](crow::websocket::Connection& conn, const std::string& reason) {
                session = nullptr;
                hostSocket = nullptr;
                inputBuffer.reset();
                outputBuffer.reset();
            })
        .onmessage([](crow::websocket::Connection& conn,
                      const std::string& data, bool is_binary) {
            if (data.length() > inputBuffer.capacity())
            {
                BMCWEB_LOG_ERROR << "Buffer overrun when writing "
                                 << data.length() << " bytes";
                conn.close("Buffer overrun");
                return;
            }

            BMCWEB_LOG_DEBUG << "Read " << data.size()
                             << " bytes from websocket";
            auto myBuffer = boost::asio::buffer(data);
            boost::asio::buffer_copy(inputBuffer.prepare(data.size()),
                                     myBuffer);
            BMCWEB_LOG_DEBUG << "commiting " << data.size()
                             << " bytes from websocket";
            inputBuffer.commit(data.size());

            BMCWEB_LOG_DEBUG << "inputbuffer size " << inputBuffer.size();
            doWrite();
        });
}
} // namespace obmc_jtag
} // namespace crow
