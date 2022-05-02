#pragma once

#include "dbus_utils.hpp"

#include <app.hpp>
#include <async_resp.hpp>
#include <error_messages.hpp>
#include <nlohmann/json.hpp>
#include <utils/collection.hpp>
#include <utils/hex_utils.hpp>
#include <utils/json_utils.hpp>

#include <vector>

namespace crow
{
namespace google_api
{
constexpr const char* hothSearchPath = "/xyz/openbmc_project";
constexpr const char* hothInterface = "xyz.openbmc_project.Control.Hoth";
constexpr const char* rotCollectionPrefix = "/google/v1/RootOfTrustCollection";

inline void getGoogleV1(const crow::Request& /*req*/,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    asyncResp->res.jsonValue["@odata.type"] =
        "#GoogleServiceRoot.v1_0_0.GoogleServiceRoot";
    asyncResp->res.jsonValue["@odata.id"] = "/google/v1";
    asyncResp->res.jsonValue["Id"] = "Google Rest RootService";
    asyncResp->res.jsonValue["Name"] = "Google Service Root";
    asyncResp->res.jsonValue["Version"] = "1.0.0";
    asyncResp->res.jsonValue["RootOfTrustCollection"]["@odata.id"] =
        rotCollectionPrefix;
}

inline void getRootOfTrustCollection(
    const crow::Request& /*req*/,
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

inline void getRootOfTrustCollectionImpl(
    const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    auto serviceResp = std::make_shared<GoogleServiceAsyncResp>(asyncResp);
    getRootOfTrustCollection(req, serviceResp);
}

using ResolvedEntityHandler = std::function<void(
    const std::string&, const std::shared_ptr<GoogleServiceAsyncResp>&,
    const ResolvedEntity&)>;

inline void
    resolveRoT(const std::string& command,
               const std::shared_ptr<GoogleServiceAsyncResp>& serviceResp,
               const std::string& rotId, ResolvedEntityHandler&& entityHandler)
{
    auto validateFunc = [command, serviceResp, rotId,
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
            if (objPath.filename() != rotId || object.second.empty())
            {
                continue;
            }

            ResolvedEntity resolvedEntity = {.id = rotId,
                                             .service = object.second[0].first,
                                             .object = object.first,
                                             .interface = hothInterface};
            entityHandler(command, serviceResp, resolvedEntity);
            return;
        }

        // Couldn't find an object with that name.  return an error
        redfish::messages::resourceNotFound(serviceResp->asyncResp->res,
                                            "#RootOfTrust.v1_0_0.RootOfTrust",
                                            rotId);
    };

    serviceResp->objMapper->getSubTree(
        DbusMethodAddr("xyz.openbmc_project.ObjectMapper",
                       "/xyz/openbmc_project/object_mapper",
                       "xyz.openbmc_project.ObjectMapper", "GetSubTree"),
        {.depth = 0, .subtree = hothSearchPath, .interfaces = {hothInterface}},
        validateFunc);
}

inline void populateRootOfTrustEntity(
    const std::string& /*unused*/,
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
    // Need to fix this later to a stabler property.
    asyncResp->res.jsonValue["Name"] = resolvedEntity.id;
    asyncResp->res.jsonValue["Description"] = "Google Root Of Trust";
    asyncResp->res.jsonValue["Actions"]["#RootOfTrust.SendCommand"]["target"] =
        "/google/v1/RootOfTrustCollection/" + resolvedEntity.id +
        "/Actions/RootOfTrust.SendCommand";

    asyncResp->res.jsonValue["Location"]["PartLocation"]["ServiceLabel"] =
        resolvedEntity.id;
    asyncResp->res.jsonValue["Location"]["PartLocation"]["LocationType"] =
        "Embedded";
}

inline void
    getRootOfTrust(const crow::Request& /*unused*/,
                   const std::shared_ptr<GoogleServiceAsyncResp>& serviceResp,
                   const std::string& param)
{
    resolveRoT("", serviceResp, param, populateRootOfTrustEntity);
}

inline void
    getRootOfTrustImpl(const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& param)
{
    getRootOfTrust(req, std::make_shared<GoogleServiceAsyncResp>(asyncResp),
                   param);
}

inline void
    invokeRoTCommand(const std::string& command,
                     const std::shared_ptr<GoogleServiceAsyncResp>& serviceResp,
                     const ResolvedEntity& resolvedEntity)
{
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp =
        serviceResp->asyncResp;

    auto handleFunc = [asyncResp](const boost::system::error_code ec,
                                  std::vector<uint8_t>& responseBytes) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "RootOfTrust.Actions.SendCommand failed: "
                             << ec.message();
            redfish::messages::internalError(asyncResp->res);
            return;
        }

        asyncResp->res.jsonValue["CommandResponse"] =
            bytesToHexString(responseBytes);
    };
    std::vector<uint8_t> bytes = hexStringToBytes(command);
    if (bytes.empty())
    {
        BMCWEB_LOG_DEBUG << "Invalid command: " << command;
        redfish::messages::actionParameterValueTypeError(command, "Command",
                                                         "SendCommand");
        return;
    }

    serviceResp->hothInterface->sendHostCommand(
        DbusMethodAddr(resolvedEntity, "SendHostCommand"), bytes, handleFunc);
}

inline void
    sendRoTCommand(const crow::Request& request,
                   const std::shared_ptr<GoogleServiceAsyncResp>& serviceResp,
                   const std::string& rotId)
{
    std::string command;
    if (!redfish::json_util::readJsonAction(
            request, serviceResp->asyncResp->res, "Command", command))
    {
        BMCWEB_LOG_DEBUG << "Missing property Command.";
        redfish::messages::actionParameterMissing(serviceResp->asyncResp->res,
                                                  "SendCommand", "Command");
        return;
    }
    resolveRoT(command, serviceResp, rotId, invokeRoTCommand);
}

inline void
    sendRoTCommandImpl(const crow::Request& request,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& rotId)
{
    sendRoTCommand(request, std::make_shared<GoogleServiceAsyncResp>(asyncResp),
                   rotId);
}

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/google/v1/")
        .methods(boost::beast::http::verb::get)(getGoogleV1);

    BMCWEB_ROUTE(app, "/google/v1/RootOfTrustCollection")
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::get)(getRootOfTrustCollectionImpl);

    BMCWEB_ROUTE(app, "/google/v1/RootOfTrustCollection/<str>")
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::get)(getRootOfTrustImpl);

    BMCWEB_ROUTE(
        app,
        "/google/v1/RootOfTrustCollection/<str>/Actions/RootOfTrust.SendCommand")
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::post)(sendRoTCommandImpl);
}

} // namespace google_api
} // namespace crow
