#pragma once

#include "utils/json_utils.hpp"

#include <app.hpp>
#include <async_resp.hpp>
#include <dbus_utility.hpp>
#include <error_messages.hpp>
#include <nlohmann/json.hpp>
#include <utils/collection.hpp>
#include <utils/hex_utils.hpp>
#include <utils/json_utils.hpp>

#include <filesystem>
#include <fstream>
#include <functional>
#include <ios>
#include <sstream>
#include <utility>
#include <vector>

namespace crow
{
namespace google_api
{

inline void fetchFile(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& filepath)
{
    std::ifstream t(filepath, std::ios::binary);
    std::stringstream buffer;
    buffer << t.rdbuf();
    t.close();

    std::string str = buffer.str();
    std::cerr << "Binary Size: " << str.size() << std::endl;
    std::string_view strData(str.data(), str.size());
    std::string output = crow::utility::base64encode(strData);

    asyncResp->res.addHeader(boost::beast::http::field::content_type,
                             "application/octet-stream");
    asyncResp->res.addHeader(
        boost::beast::http::field::content_transfer_encoding, "Base64");
    asyncResp->res.body() = std::move(output);
}

inline void handleGoogleNvmeController(
    const crow::Request&, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& controllerId)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp{std::move(asyncResp)},
         controllerId](const boost::system::error_code ec,
                       const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            // do not add err msg in redfish response, because this is not
            //     mandatory property
            BMCWEB_LOG_INFO << "DBUS error: no matched iface for NVMe " << ec
                            << "\n";
            return;
        }
        // Iterate over all retrieved ObjectPaths.
        for (const auto& [path, services] : subtree)
        {
            sdbusplus::message::object_path objPath(path);
            if (objPath.filename() != controllerId)
            {
                continue;
            }

            for (const auto& [service, interfaces] : services)
            {
                for (const std::string& interface : interfaces)
                {
                    if (interface == "xyz.openbmc_project.Nvme")
                    {
                        asyncResp->res.jsonValue["@odata.id"] =
                            crow::utility::urlFromPieces(
                                "google", "v1",
                                "NVMeSubsystemControllerCollection",
                                controllerId);
                        asyncResp->res.jsonValue["Name"] =
                            "NVMe Subsystem Controller";

                        // NVMe actions
                        asyncResp->res
                            .jsonValue["Actions"]["#Nvme.Log"]["target"] =
                            crow::utility::urlFromPieces(
                                "google", "v1",
                                "NVMeSubsystemControllerCollection",
                                controllerId, "Actions", "Nvme.Log");
                        asyncResp->res
                            .jsonValue["Actions"]["#Nvme.Identify"]["target"] =
                            crow::utility::urlFromPieces(
                                "google", "v1",
                                "NVMeSubsystemControllerCollection",
                                controllerId, "Actions", "Nvme.Identify");

                        // TODO(wltu): Link to the NVMe storage
                        asyncResp->res.jsonValue["Links"]["Storage"] =
                            crow::utility::urlFromPieces(
                                "redfish", "v1", "Systems", "system", "Storage",
                                controllerId);
                        return;
                    }
                }
            }
        }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0,
        std::array<const char*, 1>{"xyz.openbmc_project.Nvme"});
}

inline void handleGoogleNvmeControllerCollection(
    const crow::Request&, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    asyncResp->res.jsonValue["@odata.type"] =
        "#GoogleNvmeControllerCollection.GoogleNvmeControllerCollection";
    asyncResp->res.jsonValue["@odata.id"] =
        "/google/v1/NVMeSubsystemControllerCollection";
    asyncResp->res.jsonValue["Name"] = "Google NVMe Controllers";

    redfish::collection_util::getCollectionMembers(
        asyncResp, "/google/v1/NVMeSubsystemController",
        {"xyz.openbmc_project.Nvme"});
}

inline void handleGoogleNvmeControllerIdentifyAction(
    const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& controllerId)
{
    std::string arg0;

    if (!redfish::json_util::readJsonAction(req, asyncResp->res, "arg0", arg0))
    {
        return;
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp{std::move(asyncResp)}](const boost::system::error_code ec,
                                          const std::string& filepath) {
        if (ec)
        {
            // do not add err msg in redfish response, because this is not
            //     mandatory property
            BMCWEB_LOG_INFO << "Failed to call NVMe Identify " << ec << "\n";
            redfish::messages::internalError(asyncResp->res);
            return;
        }

        fetchFile(asyncResp, filepath);
        },
        "com.google.gbmc.wltu.nvme",
        "/xyz/openbmc_project/inventory/" + controllerId,
        "xyz.openbmc_project.NVMe", "Identify", arg0);
}

inline void handleGoogleNvmeControllerLogAction(
    const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& controllerId)
{
    std::string arg0;
    std::string arg1;

    if (!redfish::json_util::readJsonAction(req, asyncResp->res, "arg0", arg0,
                                            "arg1", arg1))
    {
        return;
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp{std::move(asyncResp)}](const boost::system::error_code ec,
                                          const std::string& filepath) {
        if (ec)
        {
            // do not add err msg in redfish response, because this is not
            //     mandatory property
            BMCWEB_LOG_INFO << "Failed to call NVMe Identify " << ec << "\n";
            redfish::messages::internalError(asyncResp->res);
            return;
        }

        fetchFile(asyncResp, filepath);
        },
        "com.google.gbmc.wltu.nvme",
        "/xyz/openbmc_project/inventory/" + controllerId,
        "xyz.openbmc_project.NVMe", "GetLogPage", arg0, arg1);
}

inline void requestGoogleNVMeControllerActionIdentify(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/google/v1/NVMeSubsystemControllerCollection/<str>/Actions/NVMe.Identify")
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::post)(
            handleGoogleNvmeControllerIdentifyAction);
}

inline void requestGoogleNVMeControllerActionLog(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/google/v1/NVMeSubsystemControllerCollection/<str>/Actions/NVMe.Log")
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::post)(
            handleGoogleNvmeControllerLogAction);
}

inline void requestGoogleNVMeController(App& app)
{
    BMCWEB_ROUTE(app, "/google/v1/NVMeSubsystemControllerCollection/<str>")
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::get)(handleGoogleNvmeController);
}

inline void requestGoogleNVMeControllerCollection(App& app)
{
    BMCWEB_ROUTE(app, "/google/v1/NVMeSubsystemControllerCollection")
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::get)(
            handleGoogleNvmeControllerCollection);
}

} // namespace google_api
} // namespace crow
