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

class Websocket : public std::enable_shared_from_this<Websocket>
{
  public:
    inline Websocket(const std::string& consoleName,
                     crow::websocket::Connection& conn) :
        endpoint(consoleName)
    {
        sessions.insert(&conn);
        hostSocket =
            std::make_unique<boost::asio::local::stream_protocol::socket>(
                conn.getIoContext());
    }
    ~Websocket() = default;

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
            [this, self(shared_from_this())](
                const boost::system::error_code& ec, std::size_t bytesRead) {
                BMCWEB_LOG_DEBUG << "read done.  Read " << bytesRead
                                 << " bytes";
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

    inline void connect()
    {
        this->hostSocket->async_connect(
            endpoint,
            [this, self(shared_from_this())](boost::system::error_code ec) {
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

    inline void addConnection(crow::websocket::Connection& conn)
    {
        sessions.insert(&conn);
    }

    inline bool removeConnection(crow::websocket::Connection& conn)
    {
        sessions.erase(&conn);
        return sessions.empty();
    }

    boost::asio::local::stream_protocol::endpoint endpoint;
    std::unique_ptr<boost::asio::local::stream_protocol::socket> hostSocket;
    std::array<char, 4096> outputBuffer;
    std::string inputBuffer;
    boost::container::flat_set<crow::websocket::Connection*> sessions;
    bool doingWrite = false;
};

} // namespace obmc_websocket
} // namespace crow