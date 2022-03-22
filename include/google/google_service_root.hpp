// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "dbus_utils.hpp"

#include <app.hpp>
#include <async_resp.hpp>
#include <boost/algorithm/hex.hpp>
#include <error_messages.hpp>
#include <nlohmann/json.hpp>
#include <utils/collection.hpp>
#include <utils/json_utils.hpp>

#include <vector>

namespace crow
{
namespace google_api
{
#define HOTH_SEARCH_PATH "/xyz/openbmc_project"
#define HOTH_INTERFACE "xyz.openbmc_project.Control.Hoth"
#define ROT_COLLECTION_PREFIX "/google/v1/RootOfTrustCollection"

std::shared_ptr<ObjectMapperInterface> obj_mapper;
std::shared_ptr<RedfishUtilWrapper> rf_utils;
std::shared_ptr<HothInterface> hoth_iface;

inline void getGoogleV1(const crow::Request&,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    asyncResp->res.jsonValue["@odata.type"] =
        "#GoogleServiceRoot.v1_0_0.GoogleServiceRoot";
    asyncResp->res.jsonValue["@odata.id"] = "/google/v1";
    asyncResp->res.jsonValue["Id"] = "Google Rest RootService";
    asyncResp->res.jsonValue["Name"] = "Google Service Root";
    asyncResp->res.jsonValue["Version"] = "1.0.0";
    asyncResp->res.jsonValue["RootOfTrustCollection"] = {
        {"@odata.id", ROT_COLLECTION_PREFIX}};
}

inline void getRootOfTrustCollection(
    const crow::Request&, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    asyncResp->res.jsonValue["@odata.id"] = ROT_COLLECTION_PREFIX;
    asyncResp->res.jsonValue["@odata.type"] =
        "#RootOfTrustCollection.RootOfTrustCollection";
    rf_utils->populateCollectionMembers(
        asyncResp, ROT_COLLECTION_PREFIX,
        std::vector<const char*>{HOTH_INTERFACE}, HOTH_SEARCH_PATH);
}

using ResolvedEntityHandler = std::function<void(
    const crow::Request&, const std::shared_ptr<bmcweb::AsyncResp>&,
    const ResolvedEntity&)>;

inline void resolveRoT(const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& rotId,
                       ResolvedEntityHandler&& entityHandler)
{
    auto validateFunc = [&req, asyncResp, rotId,
                         entityHandler{std::forward<ResolvedEntityHandler>(
                             entityHandler)}](
                            const boost::system::error_code ec,
                            const crow::openbmc_mapper::GetSubTreeType&
                                subtree) {
        if (ec)
        {
            redfish::messages::internalError(asyncResp->res);
            return;
        }
        // Iterate over all retrieved ObjectPaths.
        for (const std::pair<
                 std::string,
                 std::vector<std::pair<std::string, std::vector<std::string>>>>&
                 object : subtree)
        {
            sdbusplus::message::object_path objPath(object.first);
            if (objPath.filename() != rotId)
            {
                continue;
            }
            ResolvedEntity resolvedEntity = {.id = rotId,
                                             .service = object.second[0].first,
                                             .object = object.first,
                                             .interface = HOTH_INTERFACE};
            entityHandler(req, asyncResp, resolvedEntity);
            return;
        }

        // Couldn't find an object with that name.  return an error
        redfish::messages::resourceNotFound(
            asyncResp->res, "#RootOfTrust.v1_0_0.RootOfTrust", rotId);
    };
    ObjectMapperGetSubTreeParams params;
    params.depth = 0;
    params.subtree = HOTH_SEARCH_PATH;
    params.interfaces = {HOTH_INTERFACE};
    static const DbusMethodAddr addr("xyz.openbmc_project.ObjectMapper",
                                     "/xyz/openbmc_project/object_mapper",
                                     "xyz.openbmc_project.ObjectMapper",
                                     "GetSubTree");
    obj_mapper->getSubTree(addr, params, validateFunc);
}

inline void populateRootOfTrustEntity(
    const crow::Request&, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const ResolvedEntity& resolvedEntity)
{
    asyncResp->res.jsonValue["@odata.type"] = "#RootOfTrust.v1_0_0.RootOfTrust";
    asyncResp->res.jsonValue["@odata.id"] =
        "/google/v1/RootOfTrustCollection/" + resolvedEntity.id;

    asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
    asyncResp->res.jsonValue["Id"] = resolvedEntity.id;
    asyncResp->res.jsonValue["Name"] = "RootOfTrust-" + resolvedEntity.id;
    asyncResp->res.jsonValue["Actions"]["#RootOfTrust.SendCommand"]["target"] =
        "/google/v1/RootOfTrustCollection/" + resolvedEntity.id +
        "/Actions/RootOfTrust.SendCommand";

    // Need to expand this later with a call to Hoth
    nlohmann::json& protectedComponents =
        asyncResp->res.jsonValue["Links"]["ComponentsProtected"];
    protectedComponents = nlohmann::json::array();
    protectedComponents.push_back({{"@odata.id", "/google/v1"}});
    asyncResp->res.jsonValue["Links"]["ComponentsProtected@odata.count"] = 1;
}

inline void getRootOfTrust(const crow::Request& req,
                           const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& param)
{
    resolveRoT(req, asyncResp, param, populateRootOfTrustEntity);
}

inline void
    invokeRoTCommand(const crow::Request& request,
                     const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const ResolvedEntity& resolvedEntity)
{
    std::string command;
    if (!redfish::json_util::readJsonAction(request, asyncResp->res, "Command",
                                            command))
    {
        BMCWEB_LOG_DEBUG << "Missing property Command.";
        redfish::messages::actionParameterMissing(asyncResp->res, "SendCommand",
                                                  "Command");
        return;
    }

    auto handleFunc = [asyncResp](const boost::system::error_code ec,
                                  std::vector<uint8_t>& resp_bytes) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "RootOfTrust.Actions.SendCommand failed: "
                             << ec.message();
            redfish::messages::internalError(asyncResp->res);
            return;
        }

        std::string resp;
        boost::algorithm::hex(resp_bytes.begin(), resp_bytes.end(),
                              std::back_inserter(resp));
        asyncResp->res.jsonValue["CommandResponse"] = resp;
    };
    std::vector<uint8_t> bytes;
    boost::algorithm::unhex(command, std::back_inserter(bytes));

    hoth_iface->sendHostCommand(
        toDbusMethodAddr(resolvedEntity, "SendHostCommand"), bytes, handleFunc);
}

inline void sendRoTCommand(const crow::Request& request,
                           const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& rotId)
{
    resolveRoT(request, asyncResp, rotId, invokeRoTCommand);
}

inline void requestRoutes(App& app)
{
    crow::google_api::obj_mapper = std::make_shared<ObjectMapperInterface>();
    crow::google_api::rf_utils = std::make_shared<RedfishUtilWrapper>();
    crow::google_api::hoth_iface = std::make_shared<HothInterface>();
    BMCWEB_ROUTE(app, "/google/v1/")
        .methods(boost::beast::http::verb::get)(getGoogleV1);

    BMCWEB_ROUTE(app, "/google/v1/RootOfTrustCollection")
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::get)(getRootOfTrustCollection);

    BMCWEB_ROUTE(app, "/google/v1/RootOfTrustCollection/<str>")
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::get)(getRootOfTrust);

    BMCWEB_ROUTE(
        app,
        "/google/v1/RootOfTrustCollection/<str>/Actions/RootOfTrust.SendCommand")
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::post)(sendRoTCommand);
}

} // namespace google_api
} // namespace crow
