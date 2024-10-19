#pragma once

#include "filter_expr_executor.hpp"
#include "privileges.hpp"
#include "registries/privilege_registry.hpp"

#include <app.hpp>
#include <event_service_manager.hpp>

#include <memory>
#include <string>

namespace redfish
{

inline void createSubscription(crow::sse_socket::Connection& conn,
                               const crow::Request& req)
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

    std::optional<filter_ast::LogicalAnd> filter;

    boost::urls::params_base::iterator filterIt =
        req.url().params().find("$filter");

    if (filterIt != req.url().params().end())
    {
        std::string_view filterValue = (*filterIt).value;
        filter = parseFilter(filterValue);
        if (!filter)
        {
            conn.close(std::format("Bad $filter param: {}", filterValue));
            return;
        }
    }

    std::string lastEventId(req.getHeaderValue("Last-Event-Id"));

    std::shared_ptr<Subscription> subValue =
        std::make_shared<Subscription>(conn);

    if (subValue->userSub == nullptr)
    {
        BMCWEB_LOG_ERROR("Subscription data is null");
        conn.close("Internal Error");
        return;
    }

    // GET on this URI means, Its SSE subscriptionType.
    subValue->userSub->subscriptionType = redfish::subscriptionTypeSSE;

    subValue->userSub->protocol = "Redfish";
    subValue->userSub->retryPolicy = "TerminateAfterRetries";
    subValue->userSub->eventFormatType = "Event";

    std::string id = manager.addSSESubscription(subValue, lastEventId);
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
