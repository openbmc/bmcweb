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

inline void requestAggregationServiceRoutes(App& app)
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

inline void handleAggregationSourcesGet(
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

    // TODO: Query D-Bus for satellite configs and add them to the Members array
    RedfishAggregator::getSatelliteConfigs(std::bind_front(populateAggregationSources, asyncResp));
}

inline void populateAggregationSources(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::unordered_map<std::string, boost::urls::url>& satelliteInfo)
{
    nlohmann::json& json = asyncResp->res.jsonValue;
    json["Members"] = nlohmann::json::array();
    nlohmann::json& members = json["Members"];
    for (const auto& sat : satelliteInfo)
    {
        nlohmann::json::object_t member;
        member["@odata.id"] =
            "/redfish/v1/AggregationService/AggregationSources/" + sat.first;
        members.push_back(std::move(member));
    }
    json["Members@odata.count"] = members.size();
}

inline void requestAggregationSourcesRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/AggregationService/AggregationSources/")
        .privileges(redfish::privileges::getAggregationService)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleAggregationSourcesGet, std::ref(app)));
}

} // namespace redfish
