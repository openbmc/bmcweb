#pragma once

#include <app.hpp>
#include <event_service_manager.hpp>

namespace redfish
{

inline void createSubscription(crow::sse_socket::Connection& conn)
{
    if ((EventServiceManager::getInstance().getNumberOfSubscriptions() >=
         maxNoOfSubscriptions) ||
        EventServiceManager::getInstance().getNumberOfSSESubscriptions() >=
            maxNoOfSSESubscriptions)
    {
        BMCWEB_LOG_WARNING << "Max SSE subscriptions reached";
        conn.close("Max SSE subscriptions reached");
        return;
    }
    std::shared_ptr<redfish::Subscription> subValue =
        std::make_shared<redfish::Subscription>(conn);

    // GET on this URI means, Its SSE subscriptionType.
    subValue->subscriptionType = redfish::subscriptionTypeSSE;

    subValue->protocol = "Redfish";
    subValue->retryPolicy = "TerminateAfterRetries";
    subValue->eventFormatType = "Event";

    std::string id =
        redfish::EventServiceManager::getInstance().addSubscription(subValue,
                                                                    false);
    if (id.empty())
    {
        conn.close("Internal Error");
    }
}

static void deleteSubscription(crow::sse_socket::Connection& conn)
{
    redfish::EventServiceManager::getInstance().deleteSubscription(conn);
}

inline void requestRoutesEventServiceSse(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/EventService/Subscriptions/SSE")
        .privileges({{"ConfigureComponents", "ConfigureManager"}})
        .serverSentEvent()
        .onopen(createSubscription)
        .onclose(deleteSubscription);
}
} // namespace redfish
