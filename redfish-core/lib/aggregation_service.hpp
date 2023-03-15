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
    const boost::system::error_code ec,
    const std::unordered_map<std::string, boost::urls::url>& satelliteInfo)
{
    // Something went wrong while querying dbus
    if (ec)
    {
        messages::internalError(asyncResp->res);
        return;
    }
    nlohmann::json::array_t members = nlohmann::json::array();
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
    RedfishAggregator::getSatelliteConfigs(
        std::bind_front(populateAggregationSourceCollection, asyncResp));
}

inline void handleAggregationSourceCollectionHead(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/AggregationService/AggregationSourceCollection.json>; rel=describedby");
}

inline void requestRoutesAggregationSourceCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/AggregationService/AggregationSources/")
        .privileges(redfish::privileges::getAggregationSourceCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleAggregationSourceCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/AggregationService/AggregationSources/")
        .privileges(redfish::privileges::getAggregationSourceCollection)
        .methods(boost::beast::http::verb::head)(std::bind_front(
            handleAggregationSourceCollectionHead, std::ref(app)));
}

inline void populateAggregationSource(
    const std::string& aggregationSourceId,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code ec,
    const std::unordered_map<std::string, boost::urls::url>& satelliteInfo)
{
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/AggregationSource/AggregationSource.json>; rel=describedby");

    // Something went wrong while querying dbus
    if (ec)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    const auto& sat = satelliteInfo.find(aggregationSourceId);
    if (sat == satelliteInfo.end())
    {
        messages::resourceNotFound(asyncResp->res, "AggregationSource",
                                   aggregationSourceId);
        return;
    }

    asyncResp->res.jsonValue["@odata.id"] =
        crow::utility::urlFromPieces("redfish", "v1", "AggregationService",
                                     "AggregationSources", aggregationSourceId);
    asyncResp->res.jsonValue["@odata.type"] =
        "#AggregationSource.v1_3_1.AggregationSource";
    asyncResp->res.jsonValue["Id"] = aggregationSourceId;

    // TODO: We may want to change this whenever we support aggregating multiple
    // satellite BMCs.  Otherwise all AggregationSource resources will have the
    // same "Name".
    // TODO: We should use the "Name" from the satellite config whenever we add
    // support for including it in the data returned in satelliteInfo.
    asyncResp->res.jsonValue["Name"] = "Aggregation source";
    std::string hostName(sat->second.encoded_origin());
    asyncResp->res.jsonValue["HostName"] = std::move(hostName);

    // The Redfish spec requires Password to be null in responses
    asyncResp->res.jsonValue["Password"] = nullptr;
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
    RedfishAggregator::getSatelliteConfigs(std::bind_front(
        populateAggregationSource, aggregationSourceId, asyncResp));
}

inline void handleAggregationSourceHead(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& aggregationSourceId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/AggregationService/AggregationSource.json>; rel=describedby");

    // Needed to prevent unused variable error
    BMCWEB_LOG_DEBUG << "Added link header to response from "
                     << aggregationSourceId;
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
