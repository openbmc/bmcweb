#pragma once
#include <sys/socket.h>

#include <async_resp.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <websocket.hpp>

namespace crow
{

namespace obmc_websocket
{

class Websocket final : public std::enable_shared_from_this<Websocket>
{
  public:
    Websocket(std::string_view consoleName, crow::websocket::Connection& conn) :
        endpoint(consoleName), hostSocket(conn.getIoContext())
    {}
    ~Websocket() = default;

    Websocket(const Websocket&) = delete;
    Websocket& operator=(const Websocket&) = delete;
    Websocket(Websocket&&) = delete;
    Websocket& operator=(Websocket&&) = delete;

    void doWrite()
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

        doingWrite = true;
        hostSocket.async_write_some(
            boost::asio::buffer(inputBuffer.data(), inputBuffer.size()),
            [this, self(shared_from_this())](boost::beast::error_code ec,
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
                BMCWEB_LOG_ERROR << "Error in host serial write " << ec;
                return;
            }
            doWrite();
            });
    }

    void doRead()
    {
        BMCWEB_LOG_DEBUG << "Reading from socket";
        hostSocket.async_read_some(
            boost::asio::buffer(outputBuffer.data(), outputBuffer.size()),
            [this, self(shared_from_this())](
                const boost::system::error_code& ec, std::size_t bytesRead) {
            BMCWEB_LOG_DEBUG << "read done.  Read " << bytesRead << " bytes";
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Couldn't read from host serial port: "
                                 << ec;
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

    void connect()
    {
        hostSocket.async_connect(endpoint, [this, self(shared_from_this())](
                                               boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Couldn't connect to host serial port: "
                                 << ec;
                for (crow::websocket::Connection* session : sessions)
                {
                    session->close("Error in connecting to host port");
                }
                return;
            }
            doWrite();
            doRead();
        });
    }

    void addConnection(crow::websocket::Connection& conn)
    {
        sessions.insert(&conn);
    }

    bool removeConnection(crow::websocket::Connection& conn)
    {
        sessions.erase(&conn);
        return sessions.empty();
    }

    boost::asio::local::stream_protocol::endpoint endpoint;
    boost::asio::local::stream_protocol::socket hostSocket;
    std::array<char, 4096> outputBuffer{};
    std::string inputBuffer;
    boost::container::flat_set<crow::websocket::Connection*> sessions;
    bool doingWrite = false;
};

} // namespace obmc_websocket
} // namespace crow