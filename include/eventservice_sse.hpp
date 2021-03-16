#pragma once

#include <app.hpp>
#include <event_service_manager.hpp>

namespace redfish
{
namespace eventservice_sse
{

static bool createSubscription(std::shared_ptr<crow::sse::Connection>& conn,
                               const crow::Request& req, crow::Response& res)
{
    if ((EventServiceManager::getInstance().getNumberOfSubscriptions() >=
         maxNoOfSubscriptions) ||
        EventServiceManager::getInstance().getNumberOfSSESubscriptions() >=
            maxNoOfSSESubscriptions)
    {
        BMCWEB_LOG_ERROR << "Max SSE subscriptions reached";
        messages::eventSubscriptionLimitExceeded(res);
        res.end();
        return false;
    }
    BMCWEB_LOG_DEBUG << "Request query param size: " << req.urlParams.size();

    std::shared_ptr<redfish::Subscription> subValue =
        std::make_shared<redfish::Subscription>(std::move(conn));

    // GET on this URI means, Its SSE subscriptionType.
    subValue->subscriptionType = redfish::subscriptionTypeSSE;

    // TODO: parse $filter query params and fill config.
    subValue->protocol = "Redfish";
    subValue->retryPolicy = "TerminateAfterRetries";
    subValue->eventFormatType = "Event";

    std::string id =
        redfish::EventServiceManager::getInstance().addSubscription(subValue,
                                                                    false);
    if (id.empty())
    {
        messages::internalError(res);
        res.end();
        return false;
    }

    return true;
}

static void deleteSubscription(std::shared_ptr<crow::sse::Connection>& conn)
{
    redfish::EventServiceManager::getInstance().deleteSubscription(conn);
}

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/EventService/Subscriptions/SSE")
        .privileges({"ConfigureComponents", "ConfigureManager"})
        .serverSentEvent()
        .onopen([](std::shared_ptr<crow::sse::Connection>& conn,
                   const crow::Request& req, crow::Response& res) {
            BMCWEB_LOG_DEBUG << "Connection " << conn << " opened.";
            if (createSubscription(conn, req, res))
            {
                // All success, lets send SSE haader
                conn->sendSSEHeader();
            }
        })
        .onclose([](std::shared_ptr<crow::sse::Connection>& conn) {
            BMCWEB_LOG_DEBUG << "Connection " << conn << " closed";
            deleteSubscription(conn);
        });
}
} // namespace eventservice_sse
} // namespace redfish
