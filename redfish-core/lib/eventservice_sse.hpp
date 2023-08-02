#pragma once

#include <app.hpp>
#include <event_service_manager.hpp>

namespace redfish
{

inline void createSubscription(crow::sse_socket::Connection& conn)
{
    EventServiceManager& manager =
        EventServiceManager::getInstance(&conn.getIoContext());
    if ((manager.getNumberOfSubscriptions() >= maxNoOfSubscriptions) ||
        manager.getNumberOfSSESubscriptions() >= maxNoOfSSESubscriptions)
    {
        BMCWEB_LOG_WARNING("Max SSE subscriptions reached");
        conn.close("Max SSE subscriptions reached");
        return;
    }
    persistent_data::UserSubscription subValue;

    // GET on this URI means, Its SSE subscriptionType.
    subValue.subscriptionType = redfish::subscriptionTypeSSE;

    subValue.protocol = "Redfish";
    subValue.retryPolicy = "TerminateAfterRetries";
    subValue.eventFormatType = "Event";

    std::string id = manager.addSubscription(std::move(subValue), &conn);
    if (id.empty())
    {
        conn.close("Internal Error");
    }
}

inline void deleteSubscription(crow::sse_socket::Connection& conn)
{
    redfish::EventServiceManager::getInstance(&conn.getIoContext())
        .deleteSseSubscription(conn);
}

inline void requestRoutesEventServiceSse(App& app)
{
    // Note, this endpoint is given the same privilege level as creating a
    // subscription, because functionally, that's the operation being done
    BMCWEB_ROUTE(app, "/redfish/v1/EventService/SSE")
        .privileges(redfish::privileges::postEventDestinationCollection)
        .serverSentEvent()
        .onopen(createSubscription)
        .onclose(deleteSubscription);
}
} // namespace redfish
