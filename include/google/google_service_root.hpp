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
constexpr const char* hothSearchPath = "/xyz/openbmc_project";
constexpr const char* hothInterface = "xyz.openbmc_project.Control.Hoth";
constexpr const char* rotCollectionPrefix = "/google/v1/RootOfTrustCollection";

static const DbusMethodAddr
    objMapperGetSubTreeAddr("xyz.openbmc_project.ObjectMapper",
                            "/xyz/openbmc_project/object_mapper",
                            "xyz.openbmc_project.ObjectMapper", "GetSubTree");

static const std::vector<const char*> hothInterfaces = {hothInterface};
static const ObjectMapperGetSubTreeParams rotSubTreeSearchParams = {
    .depth = 0, .subtree = hothSearchPath, .interfaces = hothInterfaces};

inline void
    getGoogleV1(const crow::Request& /*unused*/,
                const std::shared_ptr<GoogleServiceAsyncResp>& serviceResp)
{
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp =
        serviceResp->asyncResp;
    asyncResp->res.jsonValue["@odata.type"] =
        "#GoogleServiceRoot.v1_0_0.GoogleServiceRoot";
    asyncResp->res.jsonValue["@odata.id"] = "/google/v1";
    asyncResp->res.jsonValue["Id"] = "Google Rest RootService";
    asyncResp->res.jsonValue["Name"] = "Google Service Root";
    asyncResp->res.jsonValue["Version"] = "1.0.0";
    asyncResp->res.jsonValue["RootOfTrustCollection"] = {
        {"@odata.id", rotCollectionPrefix}};
}

inline void getRootOfTrustCollection(
    const crow::Request& /*unused*/,
    const std::shared_ptr<GoogleServiceAsyncResp>& serviceResp)
{
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp =
        serviceResp->asyncResp;
    asyncResp->res.jsonValue["@odata.id"] = rotCollectionPrefix;
    asyncResp->res.jsonValue["@odata.type"] =
        "#RootOfTrustCollection.RootOfTrustCollection";
    serviceResp->rfUtils->populateCollectionMembers(
        asyncResp, rotCollectionPrefix, std::vector<const char*>{hothInterface},
        hothSearchPath);
}

using ResolvedEntityHandler = std::function<void(
    const crow::Request&, const std::shared_ptr<GoogleServiceAsyncResp>&,
    const ResolvedEntity&)>;

inline void
    resolveRoT(const crow::Request& req,
               const std::shared_ptr<GoogleServiceAsyncResp>& serviceResp,
               const std::string& rotId, ResolvedEntityHandler&& entityHandler)
{
    auto validateFunc = [&req, serviceResp, rotId,
                         entityHandler{std::forward<ResolvedEntityHandler>(
                             entityHandler)}](
                            const boost::system::error_code ec,
                            const crow::openbmc_mapper::GetSubTreeType&
                                subtree) {
        if (ec)
        {
            redfish::messages::internalError(serviceResp->asyncResp->res);
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
                                             .interface = hothInterface};
            entityHandler(req, serviceResp, resolvedEntity);
            return;
        }

        // Couldn't find an object with that name.  return an error
        redfish::messages::resourceNotFound(serviceResp->asyncResp->res,
                                            "#RootOfTrust.v1_0_0.RootOfTrust",
                                            rotId);
    };
    serviceResp->objMapper->getSubTree(objMapperGetSubTreeAddr,
                                       rotSubTreeSearchParams, validateFunc);
}

inline void populateRootOfTrustEntity(
    const crow::Request& /*unused*/,
    const std::shared_ptr<GoogleServiceAsyncResp>& serviceResp,
    const ResolvedEntity& resolvedEntity)
{
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp =
        serviceResp->asyncResp;
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

inline void
    getRootOfTrust(const crow::Request& req,
                   const std::shared_ptr<GoogleServiceAsyncResp>& serviceResp,
                   const std::string& param)
{
    resolveRoT(req, serviceResp, param, populateRootOfTrustEntity);
}

inline void
    invokeRoTCommand(const crow::Request& request,
                     const std::shared_ptr<GoogleServiceAsyncResp>& serviceResp,
                     const ResolvedEntity& resolvedEntity)
{
    const auto& asyncResp = serviceResp->asyncResp;
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
                                  std::vector<uint8_t>& responseBytes) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "RootOfTrust.Actions.SendCommand failed: "
                             << ec.message();
            redfish::messages::internalError(asyncResp->res);
            return;
        }

        std::string resp;
        boost::algorithm::hex(responseBytes.begin(), responseBytes.end(),
                              std::back_inserter(resp));
        asyncResp->res.jsonValue["CommandResponse"] = resp;
    };
    std::vector<uint8_t> bytes;
    boost::algorithm::unhex(command, std::back_inserter(bytes));

    serviceResp->hothInterface->sendHostCommand(
        DbusMethodAddr(resolvedEntity, "SendHostCommand"), bytes, handleFunc);
}

inline void
    sendRoTCommand(const crow::Request& request,
                   const std::shared_ptr<GoogleServiceAsyncResp>& asyncResp,
                   const std::string& rotId)
{
    resolveRoT(request, asyncResp, rotId, invokeRoTCommand);
}

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/google/v1/")
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request& request,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                getGoogleV1(request, std::make_shared<GoogleServiceAsyncResp>(
                                         asyncResp));
            });

    BMCWEB_ROUTE(app, "/google/v1/RootOfTrustCollection")
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request& request,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                getRootOfTrustCollection(
                    request,
                    std::make_shared<GoogleServiceAsyncResp>(asyncResp));
            });

    BMCWEB_ROUTE(app, "/google/v1/RootOfTrustCollection/<str>")
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request& request,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& rotId) {
                getRootOfTrust(
                    request,
                    std::make_shared<GoogleServiceAsyncResp>(asyncResp), rotId);
            });

    BMCWEB_ROUTE(
        app,
        "/google/v1/RootOfTrustCollection/<str>/Actions/RootOfTrust.SendCommand")
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& request,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& rotId) {
                sendRoTCommand(
                    request,
                    std::make_shared<GoogleServiceAsyncResp>(asyncResp), rotId);
            });
}

} // namespace google_api
} // namespace crow
