#pragma once

#include "privileges.hpp"

#include <app.hpp>
#include <event_service_manager.hpp>

namespace redfish
{
namespace eventservice_sse
{

inline bool
    createSubscription(std::shared_ptr<crow::sse_socket::Connection>& conn,
                       const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if ((EventServiceManager::getInstance().getNumberOfSubscriptions() >=
         maxNoOfSubscriptions) ||
        EventServiceManager::getInstance().getNumberOfSSESubscriptions() >=
            maxNoOfSSESubscriptions)
    {
        BMCWEB_LOG_ERROR << "Max SSE subscriptions reached";
        messages::eventSubscriptionLimitExceeded(asyncResp->res);
        return false;
    }
    BMCWEB_LOG_DEBUG << "Request query param size: "
                     << req.url().params().size();

    std::shared_ptr<redfish::Subscription> subValue =
        std::make_shared<redfish::Subscription>(std::move(conn));

    subValue->protocol = "Redfish";
    subValue->eventFormatType = "Event";

    std::string id =
        redfish::EventServiceManager::getInstance().addSubscription(subValue,
                                                                    false);
    if (id.empty())
    {
        messages::internalError(asyncResp->res);
        return false;
    }
    subValue->setSubscriptionId(id);

    return true;
}

inline void
    deleteSubscription(std::shared_ptr<crow::sse_socket::Connection>& conn)
{
    redfish::EventServiceManager::getInstance().deleteSubscription(conn);
}

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/EventService/SSE/")
        .serverSentEvent()
        .onopen([](std::shared_ptr<crow::sse_socket::Connection>& conn,
                   const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
            BMCWEB_LOG_DEBUG << "Connection " << conn << " opened.";
            if (!createSubscription(conn, req, asyncResp))
            {
                return false;
            }
            // All success, lets send SSE header
            conn->sendSSEHeader();
            return true;
        })
        .onclose([](std::shared_ptr<crow::sse_socket::Connection>& conn) {
            BMCWEB_LOG_DEBUG << "Connection " << conn << " closed";
            deleteSubscription(conn);
        });
}
} // namespace eventservice_sse
} // namespace redfish
