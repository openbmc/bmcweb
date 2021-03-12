#pragma once

#include <app.hpp>

namespace crow
{
namespace redfish
{
static constexpr const uint maxSSESessions = 4;
static boost::container::flat_set<std::shared_ptr<crow::sse::Connection>>
    sseSessions;
static std::unique_ptr<boost::asio::steady_timer> timer;

class SseSession : public std::enable_shared_from_this<SseSession>
{
  public:
    explicit SseSession(std::shared_ptr<crow::sse::Connection>& connIn) :
        conn(connIn)
    {
        if (timer == nullptr)
        {
            BMCWEB_LOG_DEBUG << "Creating timer";
            timer = std::make_unique<boost::asio::steady_timer>(
                conn->getIoContext());
            timer->expires_after(std::chrono::seconds(10));
            timer->async_wait(onTimerExpire);
        }
    }

    static void onTimerExpire(const boost::system::error_code& ec)
    {
        if (ec)
        {
            return;
        }
        for (std::shared_ptr<crow::sse::Connection> conn : sseSessions)
        {
            conn->sendEvent(
                "This is test event on ServerSentEvents connection");
        }
        timer->expires_after(std::chrono::seconds(5));
        timer->async_wait(onTimerExpire);
    }

  private:
    std::shared_ptr<crow::sse::Connection> conn;
};

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/sse")
        .privileges({"ConfigureComponents", "ConfigureManager"})
        .serverSentEvent()
        .onopen([](std::shared_ptr<crow::sse::Connection>& conn,
                   const std::shared_ptr<bmcweb::AsyncResp>&) {
            BMCWEB_LOG_DEBUG << "Connection " << conn << " opened.";
            if (sseSessions.size() == maxSSESessions)
            {
                conn->close("Max sessions are already connected");
                return;
            }

            sseSessions.insert(conn);
            SseSession obj(conn);
        })
        .onclose([](std::shared_ptr<crow::sse::Connection>& conn) {
            BMCWEB_LOG_DEBUG << "Connection " << conn << " closed";
            sseSessions.erase(conn);
        });

    BMCWEB_ROUTE(app, "/redfish/")
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&, crow::Response& res) {
                res.jsonValue = {{"v1", "/redfish/v1/"}};
                res.end();
            });
}
} // namespace redfish
} // namespace crow
