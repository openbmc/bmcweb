#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "query.hpp"
#include "utils/collection.hpp"
#include "utils/hex_utils.hpp"
#include "utils/json_utils.hpp"

#include <boost/system/error_code.hpp>
#include <nlohmann/json.hpp>

#include <array>
#include <string_view>
#include <vector>

namespace crow
{
namespace google_api
{

inline void
    handleGoogleV1Get(App& app, const crow::Request& req,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.jsonValue["@odata.type"] =
        "#GoogleServiceRoot.v1_0_0.GoogleServiceRoot";
    asyncResp->res.jsonValue["@odata.id"] = "/google/v1";
    asyncResp->res.jsonValue["Id"] = "Google Rest RootService";
    asyncResp->res.jsonValue["Name"] = "Google Service Root";
    asyncResp->res.jsonValue["Version"] = "1.0.0";
    asyncResp->res.jsonValue["RootOfTrustCollection"]["@odata.id"] =
        "/google/v1/RootOfTrustCollection";
}

inline void handleRootOfTrustCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.jsonValue["@odata.id"] = "/google/v1/RootOfTrustCollection";
    asyncResp->res.jsonValue["@odata.type"] =
        "#RootOfTrustCollection.RootOfTrustCollection";
    const std::array<std::string_view, 1> interfaces{
        "xyz.openbmc_project.Control.Hoth"};
    redfish::collection_util::getCollectionMembers(
        asyncResp, boost::urls::url("/google/v1/RootOfTrustCollection"),
        interfaces, "/xyz/openbmc_project");
}

// Helper struct to identify a resolved D-Bus object interface
struct ResolvedEntity
{
    std::string id;
    std::string service;
    std::string object;
    std::string interface;
};

using ResolvedEntityHandler = std::function<void(
    const std::string&, const std::shared_ptr<bmcweb::AsyncResp>&,
    const ResolvedEntity&)>;

inline void hothGetSubtreeCallback(
    const std::string& command,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& rotId, const ResolvedEntityHandler& entityHandler,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        redfish::messages::internalError(asyncResp->res);
        return;
    }
    for (const auto& [path, services] : subtree)
    {
        sdbusplus::message::object_path objPath(path);
        if (objPath.filename() != rotId || services.empty())
        {
            continue;
        }

        ResolvedEntity resolvedEntity = {
            .id = rotId,
            .service = services[0].first,
            .object = path,
            .interface = "xyz.openbmc_project.Control.Hoth"};
        entityHandler(command, asyncResp, resolvedEntity);
        return;
    }

    // Couldn't find an object with that name.  return an error
    redfish::messages::resourceNotFound(asyncResp->res, "RootOfTrust", rotId);
}

inline void resolveRoT(const std::string& command,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& rotId,
                       ResolvedEntityHandler&& entityHandler)
{
    constexpr std::array<std::string_view, 1> hothIfaces = {
        "xyz.openbmc_project.Control.Hoth"};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project", 0, hothIfaces,
        [command, asyncResp, rotId,
         entityHandler{std::forward<ResolvedEntityHandler>(entityHandler)}](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
        hothGetSubtreeCallback(command, asyncResp, rotId, entityHandler, ec,
                               subtree);
        });
}

inline void populateRootOfTrustEntity(
    const std::string& /*unused*/,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const ResolvedEntity& resolvedEntity)
{
    asyncResp->res.jsonValue["@odata.type"] = "#RootOfTrust.v1_0_0.RootOfTrust";
    asyncResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
        "google", "v1", "RootOfTrustCollection", resolvedEntity.id);

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
    handleRootOfTrustGet(App& app, const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& param)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    std::string emptyCommand;
    resolveRoT(emptyCommand, asyncResp, param, populateRootOfTrustEntity);
}

inline void
    invocationCallback(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const boost::system::error_code& ec,
                       const std::vector<uint8_t>& responseBytes)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR << "RootOfTrust.Actions.SendCommand failed: "
                         << ec.message();
        redfish::messages::internalError(asyncResp->res);
        return;
    }

    asyncResp->res.jsonValue["CommandResponse"] =
        bytesToHexString(responseBytes);
}

inline void
    invokeRoTCommand(const std::string& command,
                     const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const ResolvedEntity& resolvedEntity)
{
    std::vector<uint8_t> bytes = hexStringToBytes(command);
    if (bytes.empty())
    {
        BMCWEB_LOG_DEBUG << "Invalid command: " << command;
        redfish::messages::actionParameterValueTypeError(command, "Command",
                                                         "SendCommand");
        return;
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp{asyncResp}](const boost::system::error_code& ec,
                               const std::vector<uint8_t>& responseBytes) {
        invocationCallback(asyncResp, ec, responseBytes);
        },
        resolvedEntity.service, resolvedEntity.object, resolvedEntity.interface,
        "SendHostCommand", bytes);
}

inline void handleRoTSendCommandPost(
    App& app, const crow::Request& request,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& rotId)
{
    if (!redfish::setUpRedfishRoute(app, request, asyncResp))
    {
        return;
    }
    std::string command;
    if (!redfish::json_util::readJsonAction(request, asyncResp->res, "Command",
                                            command))
    {
        BMCWEB_LOG_DEBUG << "Missing property Command.";
        redfish::messages::actionParameterMissing(asyncResp->res, "SendCommand",
                                                  "Command");
        return;
    }

    resolveRoT(command, asyncResp, rotId, invokeRoTCommand);
}

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/google/v1/")
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleGoogleV1Get, std::ref(app)));

    BMCWEB_ROUTE(app, "/google/v1/RootOfTrustCollection")
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleRootOfTrustCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/google/v1/RootOfTrustCollection/<str>")
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleRootOfTrustGet, std::ref(app)));

    BMCWEB_ROUTE(
        app,
        "/google/v1/RootOfTrustCollection/<str>/Actions/RootOfTrust.SendCommand")
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleRoTSendCommandPost, std::ref(app)));
}

} // namespace google_api
} // namespace crow
