#pragma once

#include <app.hpp>
#include <event_service_manager.hpp>

namespace redfish
{
namespace eventservice_sse
{

static bool createSubscription(std::shared_ptr<crow::SseConnection>& conn,
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

    // EventService SSE supports only "$filter" query param.
    if (req.urlParams.size() > 1)
    {
        messages::invalidQueryFilter(res);
        res.end();
        return false;
    }
    std::string eventFormatType;
    std::string queryFilters;
    if (req.urlParams.size())
    {
        boost::urls::url_view::params_type::iterator it =
            req.urlParams.find("$filter");
        if (it == req.urlParams.end())
        {
            messages::invalidQueryFilter(res);
            res.end();
            return false;
        }
        queryFilters = it->value();
    }
    else
    {
        eventFormatType = "Event";
    }

    std::vector<std::string> msgIds;
    std::vector<std::string> regPrefixes;
    std::vector<std::string> mrdsArray;
    if (!queryFilters.empty())
    {
        // Reading from query params.
        bool status = readSSEQueryParams(queryFilters, eventFormatType, msgIds,
                                         regPrefixes, mrdsArray);
        if (!status)
        {
            messages::invalidObject(res, queryFilters);
            res.end();
            return false;
        }

        // RegsitryPrefix and messageIds are mutuly exclusive as per redfish
        // specification.
        if (regPrefixes.size() && msgIds.size())
        {
            messages::mutualExclusiveProperties(res, "RegistryPrefix",
                                                "MessageId");
            res.end();
            return false;
        }

        if (!eventFormatType.empty())
        {
            if (std::find(supportedEvtFormatTypes.begin(),
                          supportedEvtFormatTypes.end(),
                          eventFormatType) == supportedEvtFormatTypes.end())
            {
                messages::propertyValueNotInList(res, eventFormatType,
                                                 "EventFormatType");
                res.end();
                return false;
            }
        }
        else
        {
            // If nothing specified, using default "Event"
            eventFormatType = "Event";
        }

        if (!regPrefixes.empty())
        {
            for (const std::string& it : regPrefixes)
            {
                if (std::find(supportedRegPrefixes.begin(),
                              supportedRegPrefixes.end(),
                              it) == supportedRegPrefixes.end())
                {
                    messages::propertyValueNotInList(res, it, "RegistryPrefix");
                    res.end();
                    return false;
                }
            }
        }
    }

    std::shared_ptr<redfish::Subscription> subValue =
        std::make_shared<redfish::Subscription>(std::move(conn));

    // GET on this URI means, Its SSE subscriptionType.
    subValue->subscriptionType = subscriptionTypeSSE;
    subValue->protocol = "Redfish";
    subValue->retryPolicy = "TerminateAfterRetries";
    subValue->eventFormatType = eventFormatType;
    subValue->registryMsgIds = msgIds;
    subValue->registryPrefixes = regPrefixes;
    subValue->metricReportDefinitions = mrdsArray;

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

static void deleteSubscription(std::shared_ptr<crow::SseConnection>& conn)
{
    redfish::EventServiceManager::getInstance().deleteSubscription(conn);
}

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/EventService/Subscriptions/SSE")
        .privileges({"ConfigureComponents", "ConfigureManager"})
        .serverSentEvent()
        .onopen([](std::shared_ptr<crow::SseConnection>& conn,
                   const crow::Request& req, crow::Response& res) {
            BMCWEB_LOG_DEBUG << "Connection " << conn << " opened.";
            if (createSubscription(conn, req, res))
            {
                // All success, lets send SSE haader
                conn->sendSSEHeader();
            }
        })
        .onclose([](std::shared_ptr<crow::SseConnection>& conn) {
            BMCWEB_LOG_DEBUG << "Connection " << conn << " closed";
            deleteSubscription(conn);
        });
}
} // namespace eventservice_sse
} // namespace redfish
