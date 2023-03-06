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

    void onMessage(std::string_view data)
    {
        BMCWEB_LOG_DEBUG("conn:{}, Read {} bytes from websocket", logPtr(&conn),
                         data.size());

        conn.deferRead();
        doWrite(data);
    }

  protected:
    void afterDoRead(const std::weak_ptr<KvmSession>& weak,
                     const boost::system::error_code& ec, std::size_t bytesRead)
    {
        auto self = weak.lock();
        if (self == nullptr)
        {
            return;
        }
        BMCWEB_LOG_DEBUG("conn:{}, read done.  Read {} bytes", logPtr(&conn),
                         bytesRead);
        if (ec)
        {
            BMCWEB_LOG_ERROR("conn:{}, Couldn't read from KVM socket port: {}",
                             logPtr(&conn), ec);
            if (ec != boost::asio::error::operation_aborted)
            {
                conn.close("Error in connecting to KVM port");
            }
            return;
        }

        std::string_view payload(outputBuffer.data(), bytesRead);
        BMCWEB_LOG_DEBUG("conn:{}, Sending payload size {}", logPtr(&conn),
                         payload.size());
        conn.sendEx(crow::websocket::MessageType::Binary, payload,
                    [weak(weak_from_this())]() {
            auto self2 = weak.lock();
            if (self2 == nullptr)
            {
                return;
            }
            self2->doRead();
        });
    }

    void doRead()
    {
        BMCWEB_LOG_DEBUG("conn:{}, Reading from kvm socket", logPtr(&conn));
        hostSocket.async_read_some(
            boost::asio::buffer(outputBuffer),
            std::bind_front(&KvmSession::afterDoRead, this, weak_from_this()));
    }

    void doWrite(std::string_view data)
    {
        hostSocket.async_write_some(
            boost::asio::buffer(data),
            [this, weak(weak_from_this())](const boost::system::error_code& ec,
                                           std::size_t bytesWritten) {
            auto self = weak.lock();
            if (self == nullptr)
            {
                return;
            }
            BMCWEB_LOG_DEBUG("conn:{}, Wrote {}bytes", logPtr(&conn),
                             bytesWritten);

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

            conn.resumeRead();
        });
    }

    crow::websocket::Connection& conn;
    boost::asio::ip::tcp::socket hostSocket;
    std::array<char, 1024UL * 50UL> outputBuffer{};
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
        .onmessage(
            [](crow::websocket::Connection& conn, std::string_view data, bool) {
        if (sessions[&conn])
        {
            sessions[&conn]->onMessage(data);
        }
    });
}

} // namespace obmc_kvm
} // namespace crow
