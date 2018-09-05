#pragma once

#include <crow/app.h>

#include <boost/algorithm/string.hpp>
#include <dbus_singleton.hpp>
#include <fstream>
#include <persistent_data_middleware.hpp>
#include <streambuf>
#include <string>
#include <token_authorization_middleware.hpp>
namespace crow
{
namespace redfish
{

using ManagedObjectType = std::vector<std::pair<
    sdbusplus::message::object_path,
    boost::container::flat_map<
        std::string, boost::container::flat_map<
                         std::string, sdbusplus::message::variant<bool>>>>>;

static boost::container::flat_set<crow::SseConnection*> sessions;

static std::unique_ptr<boost::asio::steady_timer> timer;

void onTimerExpire(const boost::system::error_code& ec)
{
    BMCWEB_LOG_DEBUG << "onTimerExpire Timer expired";
    if (ec)
    {
        return;
    }
    for (crow::SseConnection* conn : sessions)
    {
        BMCWEB_LOG_DEBUG << "Sent message to " << conn;
        conn->sendEvent("Event totally happened\nTime to Dance!!@ !");
    }
    timer->expires_after(std::chrono::seconds(5));
    timer->async_wait(onTimerExpire);
}

template <typename... Middlewares> void requestRoutes(Crow<Middlewares...>& app)
{
    BMCWEB_ROUTE(app, "/sse/")
        .serverSentEvent(
            [](crow::SseConnection& conn) {
                if (timer == nullptr)
                {
                    BMCWEB_LOG_DEBUG << "Creating timer";
                    timer = make_unique<boost::asio::steady_timer>(
                        conn.getIoService());
                    timer->expires_from_now(std::chrono::seconds(5));
                    timer->async_wait(onTimerExpire);
                }
                sessions.insert(&conn);
                conn.sendEvent("Connection started");
            },
            [](crow::SseConnection& conn) {
                sessions.erase(&conn);
                timer = nullptr;
            });

    BMCWEB_ROUTE(app, "/redfish/")
        .methods("GET"_method)(
            [](const crow::Request& req, crow::Response& res) {
                res.jsonValue = {{"v1", "/redfish/v1/"}};
                res.end();
            });
}
} // namespace redfish
} // namespace crow
