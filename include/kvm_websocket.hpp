#pragma once
#include "app.hpp"
#include "async_resp.hpp"
#include "websocket.hpp"

#include <sys/socket.h>

#include <boost/container/flat_map.hpp>

namespace crow
{
namespace obmc_kvm
{

static constexpr const uint maxSessions = 4;

class KvmSession : public std::enable_shared_from_this<KvmSession>
{
  public:
    explicit KvmSession(crow::websocket::Connection& connIn) :
        conn(connIn), hostSocket(conn.getIoContext())
    {
        boost::asio::ip::tcp::endpoint endpoint(
            boost::asio::ip::make_address("127.0.0.1"), 5900);
        hostSocket.async_connect(
            endpoint, [this, &connIn](const boost::system::error_code& ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR(
                        "conn:{}, Couldn't connect to KVM socket port: {}",
                        logPtr(&conn), ec);
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
            BMCWEB_LOG_ERROR("conn:{}, Buffer overrun when writing {} bytes",
                             logPtr(&conn), data.length());
            conn.close("Buffer overrun");
            return;
        }

        BMCWEB_LOG_DEBUG("conn:{}, Read {} bytes from websocket", logPtr(&conn),
                         data.size());
        size_t copied = boost::asio::buffer_copy(
            inputBuffer.prepare(data.size()), boost::asio::buffer(data));
        BMCWEB_LOG_DEBUG("conn:{}, Committing {} bytes from websocket",
                         logPtr(&conn), copied);
        inputBuffer.commit(copied);

        BMCWEB_LOG_DEBUG("conn:{}, inputbuffer size {}", logPtr(&conn),
                         inputBuffer.size());
        doWrite();
    }

  protected:
    void doRead()
    {
        std::size_t bytes = outputBuffer.capacity() - outputBuffer.size();
        BMCWEB_LOG_DEBUG("conn:{}, Reading {} from kvm socket", logPtr(&conn),
                         bytes);
        hostSocket.async_read_some(
            outputBuffer.prepare(outputBuffer.capacity() - outputBuffer.size()),
            [this, weak(weak_from_this())](const boost::system::error_code& ec,
                                           std::size_t bytesRead) {
                auto self = weak.lock();
                if (self == nullptr)
                {
                    return;
                }
                BMCWEB_LOG_DEBUG("conn:{}, read done.  Read {} bytes",
                                 logPtr(&conn), bytesRead);
                if (ec)
                {
                    BMCWEB_LOG_ERROR(
                        "conn:{}, Couldn't read from KVM socket port: {}",
                        logPtr(&conn), ec);
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
                BMCWEB_LOG_DEBUG("conn:{}, Sending payload size {}",
                                 logPtr(&conn), payload.size());
                conn.sendBinary(payload);
                outputBuffer.consume(bytesRead);

                doRead();
            });
    }

    void doWrite()
    {
        if (doingWrite)
        {
            BMCWEB_LOG_DEBUG("conn:{}, Already writing.  Bailing out",
                             logPtr(&conn));
            return;
        }
        if (inputBuffer.size() == 0)
        {
            BMCWEB_LOG_DEBUG("conn:{}, inputBuffer empty.  Bailing out",
                             logPtr(&conn));
            return;
        }

        doingWrite = true;
        hostSocket.async_write_some(
            inputBuffer.data(),
            [this, weak(weak_from_this())](const boost::system::error_code& ec,
                                           std::size_t bytesWritten) {
                auto self = weak.lock();
                if (self == nullptr)
                {
                    return;
                }
                BMCWEB_LOG_DEBUG("conn:{}, Wrote {}bytes", logPtr(&conn),
                                 bytesWritten);
                doingWrite = false;
                inputBuffer.consume(bytesWritten);

                if (ec == boost::asio::error::eof)
                {
                    conn.close("KVM socket port closed");
                    return;
                }
                if (ec)
                {
                    BMCWEB_LOG_ERROR("conn:{}, Error in KVM socket write {}",
                                     logPtr(&conn), ec);
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
    boost::beast::flat_static_buffer<1024UL * 50UL> outputBuffer;
    boost::beast::flat_static_buffer<1024UL> inputBuffer;
    bool doingWrite{false};
};

using SessionMap = boost::container::flat_map<crow::websocket::Connection*,
                                              std::shared_ptr<KvmSession>>;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static SessionMap sessions;

inline void requestRoutes(App& app)
{
    sessions.reserve(maxSessions);

    BMCWEB_ROUTE(app, "/kvm/0")
        .privileges({{"ConfigureComponents", "ConfigureManager"}})
        .websocket()
        .onopen([](crow::websocket::Connection& conn) {
            BMCWEB_LOG_DEBUG("Connection {} opened", logPtr(&conn));

            if (sessions.size() == maxSessions)
            {
                conn.close("Max sessions are already connected");
                return;
            }

            sessions[&conn] = std::make_shared<KvmSession>(conn);
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
