#pragma once

#include "app.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "query.hpp"
#include "redfish_aggregator.hpp"
#include "registries/privilege_registry.hpp"

#include <nlohmann/json.hpp>

#include <functional>
#include <memory>

namespace redfish
{

inline void handleAggregationServiceHead(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/AggregationService/AggregationService.json>; rel=describedby");
}

inline void handleAggregationServiceGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/AggregationService/AggregationService.json>; rel=describedby");
    nlohmann::json& json = asyncResp->res.jsonValue;
    json["@odata.id"] = "/redfish/v1/AggregationService";
    json["@odata.type"] = "#AggregationService.v1_0_1.AggregationService";
    json["Id"] = "AggregationService";
    json["Name"] = "Aggregation Service";
    json["Description"] = "Aggregation Service";
    json["ServiceEnabled"] = true;
    json["AggregationSources"]["@odata.id"] =
        "/redfish/v1/AggregationService/AggregationSources";
}

inline void requestRoutesAggregationService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/AggregationService/")
        .privileges(redfish::privileges::headAggregationService)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleAggregationServiceHead, std::ref(app)));
    BMCWEB_ROUTE(app, "/redfish/v1/AggregationService/")
        .privileges(redfish::privileges::getAggregationService)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleAggregationServiceGet, std::ref(app)));
}

inline void populateAggregationSourceCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::unordered_map<std::string, boost::urls::url>& satelliteInfo)
{
    nlohmann::json members = nlohmann::json::array();
    for (const auto& sat : satelliteInfo)
    {
        nlohmann::json::object_t member;
        member["@odata.id"] =
            crow::utility::urlFromPieces("redfish", "v1", "AggregationService",
                                         "AggregationSources", sat.first);
        members.push_back(std::move(member));
    }
    asyncResp->res.jsonValue["Members@odata.count"] = members.size();
    asyncResp->res.jsonValue["Members"] = std::move(members);
}

inline void handleAggregationSourceCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/AggregationSourceCollection/AggregationSourceCollection.json>; rel=describedby");
    nlohmann::json& json = asyncResp->res.jsonValue;
    json["@odata.id"] = "/redfish/v1/AggregationService/AggregationSources";
    json["@odata.type"] =
        "#AggregationSourceCollection.AggregationSourceCollection";
    json["Name"] = "Aggregation Source Collection";

    // Query D-Bus for satellite configs and add them to the Members array
    std::function<void(
        const std::unordered_map<std::string, boost::urls::url>&)>
        cb = std::bind_front(populateAggregationSourceCollection, asyncResp);
    RedfishAggregator::getSatelliteConfigs(cb);
}

inline void requestRoutesAggregationSourceCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/AggregationService/AggregationSources/")
        .privileges(redfish::privileges::getAggregationSourceCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleAggregationSourceCollectionGet, std::ref(app)));
}

inline void populateAggregationSource(
    const std::string& aggregationSourceId,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::unordered_map<std::string, boost::urls::url>& satelliteInfo)
{
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/AggregationSource/AggregationSource.json>; rel=describedby");

    const auto& sat = satelliteInfo.find(aggregationSourceId);
    if (sat == satelliteInfo.end())
    {
        messages::resourceNotFound(asyncResp->res, "AggregationSource",
                                   aggregationSourceId);
    }

    asyncResp->res.jsonValue["@odata.id"] =
        crow::utility::urlFromPieces("redfish", "v1", "AggregationService",
                                     "AggregationSources", aggregationSourceId);
    asyncResp->res.jsonValue["@odata.type"] =
        "#AggregationSource.v1_3_1.AggregationSource";
    asyncResp->res.jsonValue["Id"] = aggregationSourceId;

    // TODO: Should this instead be the Name property from the satellite config?
    asyncResp->res.jsonValue["Name"] = aggregationSourceId;
    std::string hostName(sat->second.scheme());
    hostName += "://";
    hostName += sat->second.encoded_host_and_port();
    asyncResp->res.jsonValue["HostName"] = std::move(hostName);

    // These are null since aggregation does not currently support authorization
    asyncResp->res.jsonValue["Password"] = "";
    asyncResp->res.jsonValue["UserName"] = "";
}

inline void handleAggregationSourceGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& aggregationSourceId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    // Query D-Bus for satellite config corresponding to the specified
    // AggregationSource
    std::function<void(
        const std::unordered_map<std::string, boost::urls::url>&)>
        cb = std::bind_front(populateAggregationSource, aggregationSourceId,
                             asyncResp);
    RedfishAggregator::getSatelliteConfigs(cb);
}

inline void requestRoutesAggregationSource(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/AggregationService/AggregationSources/<str>/")
        .privileges(redfish::privileges::getAggregationSource)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleAggregationSourceGet, std::ref(app)));
}

} // namespace redfish
