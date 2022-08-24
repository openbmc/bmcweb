#pragma once

#include "utils/json_utils.hpp"

#include <app.hpp>
#include <async_resp.hpp>
#include <dbus_utility.hpp>
#include <error_messages.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/message/native_types.hpp>
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

using NVMeControllerHandlerCb = std::function<void(
    const std::shared_ptr<bmcweb::AsyncResp>&, const std::string&,
    const std::string&, const boost::system::error_code,
    const std::vector<std::string>&)>;

// $ curl  -u root:0penBmc -X GET
// http://localhost:8080/google/v1/NVMeSubsystemController/test0
// {
//   "@odata.id": "/google/v1/NVMeSubsystemController/test0",
//   "Actions": {
//     "#Nvme.Identify": {
//       "target":
//       "/google/v1/NVMeSubsystemController/test0/Actions/Nvme.Identify"
//     },
//     "#Nvme.GetLogPage": {
//       "target":
//       "/google/v1/NVMeSubsystemController/test0/Actions/Nvme.Log"
//     }
//   },
//   "Links": {
//     "Storage": "/redfish/v1/Systems/system/Storage/test0"
//   },
//   "Name": "NVMe Subsystem Controller"
// }

// xyz.openbmc_project.Configuration.NVME1000
inline void fetchFile(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const sdbusplus::message::unix_fd& fd)
{
    int dupFd = dup(fd.fd);
    FILE* f = fdopen(dupFd, "rb");
    if (!f)
    {
        BMCWEB_LOG_ERROR << "Failed to open file descriptor " << fd.fd;
        redfish::messages::internalError(asyncResp->res);
        return;
    }

    fseek(f, 0, SEEK_END);
    size_t lSize = static_cast<size_t>(ftell(f));
    rewind(f);
    std::string buff(lSize, ' ');
    if (fread(buff.data(), 1, lSize, f) == 0)
    {
        BMCWEB_LOG_ERROR << "Failed to read file descriptor " << fd.fd;
        redfish::messages::internalError(asyncResp->res);
        return;
    }

    BMCWEB_LOG_INFO << "Binary Size: " << buff.size() << "\n";
    asyncResp->res.addHeader(boost::beast::http::field::content_type,
                             "application/octet-stream");
    asyncResp->res.body() = std::move(buff);
}

inline void
    populateNVMeController(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& nvme,
                           const std::string& controllerId,
                           const boost::system::error_code ec,
                           const std::vector<std::string>& controllerList)
{
    if (ec || controllerList.empty())
    {
        redfish::messages::internalError(asyncResp->res);
        return;
    }

    std::string controllerPath;
    for (const std::string& controller : controllerList)
    {
        if (sdbusplus::message::object_path(controller).filename() ==
            controllerId)
        {
            controllerPath = controller;
            break;
        }
    }

    if (controllerPath.empty())
    {
        redfish::messages::internalError(asyncResp->res);
        return;
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp, controllerPath, nvme,
         controllerId](const boost::system::error_code ec2,
                       const dbus::utility::MapperGetObject& objects) {
        if (ec2)
        {
            BMCWEB_LOG_ERROR << "GetObject for path " << controllerPath
                             << " failed with code " << ec2;
            return;
        }

        if (objects.size() != 1)
        {
            redfish::messages::internalError(asyncResp->res);
            BMCWEB_LOG_ERROR << "Service supporting " << controllerPath
                             << " is not equal to 1";
            return;
        }

        asyncResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
            "google", "v1", "NVMe", nvme, "controllers", controllerId);
        asyncResp->res.jsonValue["Name"] = "Google NVMe Controller";

        // NVMe actions
        asyncResp->res.jsonValue["Actions"]["#Nvme.GetLogPage"]["target"] =
            crow::utility::urlFromPieces("google", "v1", "NVMe", nvme,
                                         "controllers", controllerId, "Actions",
                                         "#NVMe.GetLogPage");
        asyncResp->res.jsonValue["Actions"]["#Nvme.GetIdentify"]["target"] =
            crow::utility::urlFromPieces("google", "v1", "NVMe", nvme,
                                         "controllers", controllerId, "Actions",
                                         "NVMe.GetIdentify");

        asyncResp->res.jsonValue["NVMe"]["@odata.id"] =
            crow::utility::urlFromPieces("google", "v1", "NVMe", nvme);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", controllerPath,
        std::array<const char*, 1>{"xyz.openbmc_project.Nvme.NVMeAdmin"});
}

inline void populateNVMeControllerCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& nvme, const boost::system::error_code ec,
    const std::vector<std::string>& controllerList)
{

    if (ec == boost::system::errc::io_error || controllerList.empty())
    {
        asyncResp->res.jsonValue["Controllers"] = nlohmann::json::array();
        asyncResp->res.jsonValue["Controllers@odata.count"] = 0;
        return;
    }
    if (ec)
    {
        BMCWEB_LOG_DEBUG << "DBUS response error " << ec.value();
        redfish::messages::internalError(asyncResp->res);
        return;
    }

    nlohmann::json& members = asyncResp->res.jsonValue["Controllers"];
    members = nlohmann::json::array();
    for (const std::string& controller : controllerList)
    {
        std::string name =
            sdbusplus::message::object_path(controller).filename();
        if (name.empty())
        {
            continue;
        }
        nlohmann::json::object_t member;
        member["@odata.id"] = crow::utility::urlFromPieces(
            "google", "v1", "NVMe", nvme, "controllers", name);
        members.push_back(std::move(member));
    }
    asyncResp->res.jsonValue["Controllers@odata.count"] = members.size();
}

inline void handleGoogleNvmeController(
    const crow::Request&, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& nvme, const std::string& controllerId,
    NVMeControllerHandlerCb&& cb)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp{std::move(asyncResp)}, nvme, controllerId,
         cb{std::forward<NVMeControllerHandlerCb>(cb)}](
            const boost::system::error_code ec,
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
            if (sdbusplus::message::object_path(path).filename() != nvme)
            {
                continue;
            }

            for (const auto& [service, interfaces] : services)
            {
                for (const std::string& interface : interfaces)
                {
                    if (interface ==
                        "xyz.openbmc_project.Configuration.NVME1000")
                    {
                        sdbusplus::asio::getProperty<std::vector<std::string>>(
                            *crow::connections::systemBus,
                            "xyz.openbmc_project.ObjectMapper",
                            path + "/controllers",
                            "xyz.openbmc_project.Association", "endpoints",
                            [asyncResp, nvme, controllerId,
                             cb](const boost::system::error_code ec2,
                                 const std::vector<std::string>&
                                     controllerList) {
                            cb(asyncResp, nvme, controllerId, ec2,
                               controllerList);
                            });
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
        std::array<const char*, 1>{
            "xyz.openbmc_project.Configuration.NVME1000"});
}

inline void populateNVMe(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& path, const std::string& nvme)
{
    asyncResp->res.jsonValue["@odata.id"] =
        crow::utility::urlFromPieces("google", "v1", "NVMe", nvme);
    asyncResp->res.jsonValue["Name"] = "Google NVMe";

    sdbusplus::asio::getProperty<std::vector<std::string>>(
        *crow::connections::systemBus, "xyz.openbmc_project.ObjectMapper",
        path + "/storage", "xyz.openbmc_project.Association", "endpoints",
        [asyncResp, nvme](const boost::system::error_code ec,
                          const std::vector<std::string>& storageList) {
        if (ec)
        {
            return;
        }
        if (storageList.empty())
        {
            return;
        }
        if (storageList.size() > 1)
        {
            BMCWEB_LOG_DEBUG << nvme
                             << " is associated with mutliple storages ";
            return;
        }

        sdbusplus::message::object_path storagePath(storageList[0]);
        std::string storageId = storagePath.filename();
        if (storageId.empty())
        {
            BMCWEB_LOG_ERROR << "filename() is empty in " << storagePath.str;
            return;
        }
        asyncResp->res.jsonValue["Storage"]["@odata.id"] =
            crow::utility::urlFromPieces("redfish", "v1", "Systems", "system",
                                         "Storage", storageId);
        });

    sdbusplus::asio::getProperty<std::vector<std::string>>(
        *crow::connections::systemBus, "xyz.openbmc_project.ObjectMapper",
        path + "/controllers", "xyz.openbmc_project.Association", "endpoints",
        [asyncResp, nvme](const boost::system::error_code ec,
                          const std::vector<std::string>& controllerList) {
        populateNVMeControllerCollection(asyncResp, nvme, ec, controllerList);
        });
}

inline void
    handleGoogleNvme(const crow::Request&,
                     const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& nvme)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp{std::move(asyncResp)},
         nvme](const boost::system::error_code ec,
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
            if (sdbusplus::message::object_path(path).filename() != nvme)
            {
                continue;
            }

            for (const auto& [service, interfaces] : services)
            {
                for (const std::string& interface : interfaces)
                {
                    if (interface ==
                        "xyz.openbmc_project.Configuration.NVME1000")
                    {
                        populateNVMe(asyncResp, path, nvme);
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
        std::array<const char*, 1>{
            "xyz.openbmc_project.Configuration.NVME1000"});
}

inline void handleGoogleNvmeCollection(
    const crow::Request&, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    asyncResp->res.jsonValue["@odata.type"] =
        "#GoogleNvmeCollection.GoogleNvmeCollection";
    asyncResp->res.jsonValue["@odata.id"] = "/google/v1/NVMe";
    asyncResp->res.jsonValue["Name"] = "Google NVMe Collection";

    redfish::collection_util::getCollectionMembers(
        asyncResp, "/google/v1/NVMe",
        {"xyz.openbmc_project.Configuration.NVME1000"});
}

// $ curl  -u root:0penBmc -X POST
// http://localhost:8080/google/v1/NVMeSubsystemController/test0/Actions/NVMe.GetIdentify
// -d '{"ControllerNamespace": "hello", "ControllerId": "world"}' -H
// "Content-Type: application/json"
inline void getIdentity(const std::string& controllerNamespace,
                        const std::string& controllerId,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string&, const std::string& id,
                        const boost::system::error_code ec,
                        const std::vector<std::string>& controllerList)
{
    if (ec || controllerList.empty())
    {
        redfish::messages::internalError(asyncResp->res);
        return;
    }

    std::string controllerPath;
    for (const std::string& controller : controllerList)
    {
        if (sdbusplus::message::object_path(controller).filename() == id)
        {
            controllerPath = controller;
            break;
        }
    }

    if (controllerPath.empty())
    {
        redfish::messages::internalError(asyncResp->res);
        return;
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp, controllerNamespace, controllerId,
         controllerPath](const boost::system::error_code ec2,
                         const dbus::utility::MapperGetObject& objects) {
        if (ec2)
        {
            BMCWEB_LOG_ERROR << "GetObject for path " << controllerPath
                             << " failed with code " << ec2;
            return;
        }

        if (objects.size() != 1)
        {
            redfish::messages::internalError(asyncResp->res);
            BMCWEB_LOG_ERROR << "Service supporting " << controllerPath
                             << " is not equal to 1";
            return;
        }

        crow::connections::systemBus->async_method_call(
            [asyncResp{std::move(asyncResp)}](
                const boost::system::error_code ec3,
                const sdbusplus::message::unix_fd& fd) {
            if (ec3)
            {
                // do not add err msg in redfish response, because this is not
                // mandatory property
                BMCWEB_LOG_INFO << "Failed to call NVMe Log " << ec3 << "\n";
                redfish::messages::internalError(asyncResp->res);
                return;
            }

            fetchFile(asyncResp, fd);
            },
            objects[0].first, controllerPath,
            "xyz.openbmc_project.Nvme.NVMeAdmin", "Identify",
            controllerNamespace, controllerId);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", controllerPath,
        std::array<const char*, 1>{"xyz.openbmc_project.Nvme.NVMeAdmin"});
}

inline void handleGoogleNvmeControllerIdentifyAction(
    const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& nvme, const std::string& id)
{
    std::string controllerNamespace;
    std::string controllerId;

    if (!redfish::json_util::readJsonAction(
            req, asyncResp->res, "ControllerNamespace", controllerNamespace,
            "ControllerId", controllerId))
    {
        redfish::messages::actionParameterMissing(
            asyncResp->res, "ControllerNamespace", "ControllerId");
        return;
    }
    handleGoogleNvmeController(
        req, asyncResp, nvme, id,
        [controllerNamespace, controllerId](
            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp2,
            const std::string& nvme2, const std::string& controllerIds2,
            const boost::system::error_code ec2,
            const std::vector<std::string>& controllerList2) {
        getIdentity(controllerNamespace, controllerId, asyncResp2, nvme2,
                    controllerIds2, ec2, controllerList2);
        });
}

// $ curl  -u root:0penBmc -X POST
// http://localhost:8080/google/v1/NVMeSubsystemController/test0/Actions/NVMe.GetLogPage
// -d '{"LogSpecificId": "hello", "LogId": "world"}' -H
// "Content-Type: application/json"
inline void getLog(const std::string& logSpecificId, const std::string& logId,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string&, const std::string& controllerId,
                   const boost::system::error_code ec,
                   const std::vector<std::string>& controllerList)
{
    if (ec || controllerList.empty())
    {
        redfish::messages::internalError(asyncResp->res);
        return;
    }

    std::string controllerPath;
    for (const std::string& controller : controllerList)
    {
        if (sdbusplus::message::object_path(controller).filename() ==
            controllerId)
        {
            controllerPath = controller;
            break;
        }
    }

    if (controllerPath.empty())
    {
        redfish::messages::internalError(asyncResp->res);
        return;
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp, logSpecificId, logId,
         controllerPath](const boost::system::error_code ec2,
                         const dbus::utility::MapperGetObject& objects) {
        if (ec2)
        {
            BMCWEB_LOG_ERROR << "GetObject for path " << controllerPath
                             << " failed with code " << ec2;
            return;
        }

        if (objects.size() != 1)
        {
            redfish::messages::internalError(asyncResp->res);
            BMCWEB_LOG_ERROR << "Service supporting " << controllerPath
                             << " is not equal to 1";
            return;
        }

        crow::connections::systemBus->async_method_call(
            [asyncResp{std::move(asyncResp)}](
                const boost::system::error_code ec3,
                const sdbusplus::message::unix_fd& fd) {
            if (ec3)
            {
                // do not add err msg in redfish response, because this is not
                // mandatory property
                BMCWEB_LOG_INFO << "Failed to call NVMe Log " << ec3 << "\n";
                redfish::messages::internalError(asyncResp->res);
                return;
            }

            fetchFile(asyncResp, fd);
            },
            objects[0].first, controllerPath,
            "xyz.openbmc_project.Nvme.NVMeAdmin", "GetLogPage", logSpecificId,
            logId);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", controllerPath,
        std::array<const char*, 1>{"xyz.openbmc_project.Nvme.NVMeAdmin"});
}

inline void handleGoogleNvmeControllerLogAction(
    const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& nvme, const std::string& controllerId)
{
    std::string logSpecificId;
    std::string logId;

    if (!redfish::json_util::readJsonAction(req, asyncResp->res,
                                            "LogSpecificId", logSpecificId,
                                            "LogId", logId))
    {
        redfish::messages::actionParameterMissing(asyncResp->res,
                                                  "LogSpecificId", "LogId");
        return;
    }
    handleGoogleNvmeController(
        req, asyncResp, nvme, controllerId,
        [logSpecificId,
         logId](const std::shared_ptr<bmcweb::AsyncResp>& asyncResp2,
                const std::string& nvme2, const std::string& controllerIds2,
                const boost::system::error_code ec2,
                const std::vector<std::string>& controllerList2) {
        getLog(logSpecificId, logId, asyncResp2, nvme2, controllerIds2, ec2,
               controllerList2);
        });
}

inline void requestGoogleNVMeControllerActionIdentify(App& app)
{
    BMCWEB_ROUTE(
        app, "/google/v1/NVMe/<str>/controllers/<str>/Actions/NVMe.GetIdentify")
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::post)(
            handleGoogleNvmeControllerIdentifyAction);
}

inline void requestGoogleNVMeControllerActionLog(App& app)
{
    BMCWEB_ROUTE(
        app, "/google/v1/NVMe/<str>/controllers/<str>/Actions/NVMe.GetLogPage")
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::post)(
            handleGoogleNvmeControllerLogAction);
}

inline void requestGoogleNVMeController(App& app)
{
    BMCWEB_ROUTE(app, "/google/v1/NVMe/<str>/controllers/<str>")
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& nvme, const std::string& controllerId) {
        handleGoogleNvmeController(req, asyncResp, nvme, controllerId,
                                   populateNVMeController);
        });
}

inline void requestGoogleNVMe(App& app)
{
    BMCWEB_ROUTE(app, "/google/v1/NVMe/<str>")
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::get)(handleGoogleNvme);
}

inline void requestGoogleNVMeCollection(App& app)
{
    BMCWEB_ROUTE(app, "/google/v1/NVMe")
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::get)(handleGoogleNvmeCollection);
}

} // namespace google_api
} // namespace crow
