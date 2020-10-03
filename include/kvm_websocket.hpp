#pragma once
#include <sys/socket.h>

#include <app.hpp>
#include <async_resp.hpp>
#include <boost/container/flat_map.hpp>
#include <websocket.hpp>

namespace crow
{
namespace obmc_kvm
{

static constexpr const uint maxSessions = 4;

class KvmSession
{
  public:
    explicit KvmSession(crow::websocket::Connection& connIn) :
        conn(connIn), hostSocket(conn.getIoContext()), doingWrite(false)
    {
        boost::asio::ip::tcp::endpoint endpoint(
            boost::asio::ip::make_address("127.0.0.1"), 5900);
        hostSocket.async_connect(
            endpoint, [this, &connIn](const boost::system::error_code& ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR
                        << "conn:" << &conn
                        << ", Couldn't connect to KVM socket port: " << ec;
                    if (ec != boost::asio::error::operation_aborted)
                    {
                        connIn.close("Error in connecting to KVM port");
                    }
                    return;
                }

                doRead();
            });
    }

    void onMessage(const std::string& data)
    {
        if (data.length() > inputBuffer.capacity())
        {
            BMCWEB_LOG_ERROR << "conn:" << &conn
                             << ", Buffer overrun when writing "
                             << data.length() << " bytes";
            conn.close("Buffer overrun");
            return;
        }

        BMCWEB_LOG_DEBUG << "conn:" << &conn << ", Read " << data.size()
                         << " bytes from websocket";
        boost::asio::buffer_copy(inputBuffer.prepare(data.size()),
                                 boost::asio::buffer(data));
        BMCWEB_LOG_DEBUG << "conn:" << &conn << ", Committing " << data.size()
                         << " bytes from websocket";
        inputBuffer.commit(data.size());

        BMCWEB_LOG_DEBUG << "conn:" << &conn << ", inputbuffer size "
                         << inputBuffer.size();
        doWrite();
    }

  protected:
    void doRead()
    {
        std::size_t bytes = outputBuffer.capacity() - outputBuffer.size();
        BMCWEB_LOG_DEBUG << "conn:" << &conn << ", Reading " << bytes
                         << " from kvm socket";
        hostSocket.async_read_some(
            outputBuffer.prepare(outputBuffer.capacity() - outputBuffer.size()),
            [this](const boost::system::error_code& ec, std::size_t bytesRead) {
                BMCWEB_LOG_DEBUG << "conn:" << &conn << ", read done.  Read "
                                 << bytesRead << " bytes";
                if (ec)
                {
                    BMCWEB_LOG_ERROR
                        << "conn:" << &conn
                        << ", Couldn't read from KVM socket port: " << ec;
                    if (ec != boost::asio::error::operation_aborted)
                    {
                        conn.close("Error in connecting to KVM port");
                    }
                    return;
                }

                outputBuffer.commit(bytesRead);
                std::string_view payload(
                    static_cast<const char*>(outputBuffer.data().data()),
                    bytesRead);
                BMCWEB_LOG_DEBUG << "conn:" << &conn
                                 << ", Sending payload size " << payload.size();
                conn.sendBinary(payload);
                outputBuffer.consume(bytesRead);

                doRead();
            });
    }

    void doWrite()
    {
        if (doingWrite)
        {
            BMCWEB_LOG_DEBUG << "conn:" << &conn
                             << ", Already writing.  Bailing out";
            return;
        }
        if (inputBuffer.size() == 0)
        {
            BMCWEB_LOG_DEBUG << "conn:" << &conn
                             << ", inputBuffer empty.  Bailing out";
            return;
        }

        doingWrite = true;
        hostSocket.async_write_some(
            inputBuffer.data(), [this](const boost::system::error_code& ec,
                                       std::size_t bytesWritten) {
                BMCWEB_LOG_DEBUG << "conn:" << &conn << ", Wrote "
                                 << bytesWritten << "bytes";
                doingWrite = false;
                inputBuffer.consume(bytesWritten);

                if (ec == boost::asio::error::eof)
                {
                    conn.close("KVM socket port closed");
                    return;
                }
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "conn:" << &conn
                                     << ", Error in KVM socket write " << ec;
                    if (ec != boost::asio::error::operation_aborted)
                    {
                        conn.close("Error in reading to host port");
                    }
                    return;
                }

                doWrite();
            });
    }

    crow::websocket::Connection& conn;
    boost::asio::ip::tcp::socket hostSocket;
    boost::beast::flat_static_buffer<1024U * 50U> outputBuffer;
    boost::beast::flat_static_buffer<1024U> inputBuffer;
    bool doingWrite;
};

static boost::container::flat_map<crow::websocket::Connection*,
                                  std::unique_ptr<KvmSession>>
    sessions;

inline void requestRoutes(App& app)
{
    sessions.reserve(maxSessions);

    BMCWEB_ROUTE(app, "/kvm/0")
        .privileges({"ConfigureComponents", "ConfigureManager"})
        .websocket()
        .onopen([](crow::websocket::Connection& conn,
                   const std::shared_ptr<bmcweb::AsyncResp>&) {
            BMCWEB_LOG_DEBUG << "Connection " << &conn << " opened";

            if (sessions.size() == maxSessions)
            {
                conn.close("Max sessions are already connected");
                return;
            }

            sessions[&conn] = std::make_unique<KvmSession>(conn);
        })
        .onclose([](crow::websocket::Connection& conn, const std::string&) {
            sessions.erase(&conn);
        })
        .onmessage([](crow::websocket::Connection& conn,
                      const std::string& data, bool) {
            if (sessions[&conn])
            {
                sessions[&conn]->onMessage(data);
            }
        });
}

} // namespace obmc_kvm
} // namespace crow
