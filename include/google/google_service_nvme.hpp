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
#include <utils/json_utils.hpp>

#include <functional>
#include <vector>

namespace crow
{
namespace google_api
{

using NVMeControllerHandlerCb = std::function<void(
    const boost::system::error_code, const std::vector<std::string>&)>;

inline void fetchFile(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const sdbusplus::message::unix_fd& fd)
{
    int dupFd = dup(fd.fd);
    std::string output = "testing: hello";
    BMCWEB_LOG_DEBUG << "Start Test Logs";
    while (true)
    {
        std::string temp(128, ' ');

        auto readSize = read(dupFd, temp.data(), temp.size());

        if (readSize <= 0)
        {
            BMCWEB_LOG_DEBUG << "Exit Test Logs: " << output.size();
            break;
        }

        BMCWEB_LOG_DEBUG << "Test Hi: " << temp;
        BMCWEB_LOG_DEBUG << "Test Hi Size: " << readSize;
        std::string_view tempView = temp;
        if (static_cast<size_t>(readSize) < temp.size())
        {
            tempView = tempView.substr(0, static_cast<size_t>(readSize));
        }

        BMCWEB_LOG_DEBUG << "Test Logs: " << tempView;
        output.insert(output.end(), tempView.begin(), tempView.end());
        BMCWEB_LOG_DEBUG << "Test Sleep: " << output;
    }
    close(dupFd);
    close(fd.fd);
    asyncResp->res.addHeader(boost::beast::http::field::content_type,
                             "application/octet-stream");
    asyncResp->res.body() = std::move(output);
}

inline void
    populateNVMeController(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& nvmeId,
                           const std::string& controllerId,
                           const std::string& controllerPath)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, nvmeId, controllerId,
         controllerPath](const boost::system::error_code ec,
                         const dbus::utility::MapperGetObject& objects) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "GetObject for path " << controllerPath
                             << " failed with code " << ec;
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
            "google", "v1", "NVMe", nvmeId, "controllers", controllerId);
        asyncResp->res.jsonValue["Name"] = "Google NVMe Controller";

        // NVMe actions
        asyncResp->res.jsonValue["Actions"]["#Nvme.GetLogPage"]["target"] =
            crow::utility::urlFromPieces("google", "v1", "NVMe", nvmeId,
                                         "controllers", controllerId, "Actions",
                                         "NVMe.GetLogPage");
        asyncResp->res.jsonValue["Actions"]["#Nvme.GetIdentify"]["target"] =
            crow::utility::urlFromPieces("google", "v1", "NVMe", nvmeId,
                                         "controllers", controllerId, "Actions",
                                         "NVMe.GetIdentify");

        asyncResp->res.jsonValue["NVMe"]["@odata.id"] =
            crow::utility::urlFromPieces("google", "v1", "NVMe", nvmeId);

        // WIP
        sdbusplus::asio::getProperty<std::vector<std::string>>(
            *crow::connections::systemBus, "xyz.openbmc_project.ObjectMapper",
            controllerPath + "/storageController",
            "xyz.openbmc_project.Association", "endpoints",
            [asyncResp,
             nvmeId](const boost::system::error_code ec2,
                     const std::vector<std::string>& storageControllerList) {
            if (ec2)
            {
                return;
            }
            if (storageControllerList.empty())
            {
                return;
            }
            if (storageControllerList.size() > 1)
            {
                BMCWEB_LOG_DEBUG
                    << nvmeId
                    << " is associated with mutliple storageControllers ";
                return;
            }

            sdbusplus::message::object_path storageControllerPath(
                storageControllerList[0]);
            std::string storageControllerId = storageControllerPath.filename();
            if (storageControllerId.empty())
            {
                BMCWEB_LOG_ERROR << "filename() is empty in "
                                 << storageControllerPath.str;
                return;
            }
            // Link to Storage first... before the storage controller.
            asyncResp->res.jsonValue["Storage"]["@odata.id"] =
                crow::utility::urlFromPieces("redfish", "v1", "Systems",
                                             "system", "Storage",
                                             storageControllerId);
            });
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", controllerPath,
        std::array<const char*, 1>{"xyz.openbmc_project.Nvme.NVMeAdmin"});
}

inline void
    setupNVMeController(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& nvmeId,
                        const std::string& controllerId,
                        const boost::system::error_code ec,
                        const std::vector<std::string>& controllerList)
{
    if (ec || controllerList.empty())
    {
        redfish::messages::internalError(asyncResp->res);
        return;
    }

    auto controllerIt =
        std::find_if(controllerList.begin(), controllerList.end(),
                     [controllerId](const std::string& controller) {
        return sdbusplus::message::object_path(controller).filename() ==
               controllerId;
        });

    if (controllerIt == controllerList.end())
    {
        redfish::messages::internalError(asyncResp->res);
        return;
    }

    const std::string& controllerPath = *controllerIt;
    populateNVMeController(asyncResp, nvmeId, controllerId, controllerPath);
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

void handleGoogleNvmeController(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& nvme, NVMeControllerHandlerCb&& cb)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, nvme, cb{std::forward<NVMeControllerHandlerCb>(cb)}](
            const boost::system::error_code ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            // do not add err msg in redfish response, because this is not
            // mandatory property
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
                            "xyz.openbmc_project.Association", "endpoints", cb);
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
                         const std::string& path, const std::string& nvmeId)
{
    asyncResp->res.jsonValue["@odata.id"] =
        crow::utility::urlFromPieces("google", "v1", "NVMe", nvmeId);
    asyncResp->res.jsonValue["Name"] = "Google NVMe";

    sdbusplus::asio::getProperty<std::vector<std::string>>(
        *crow::connections::systemBus, "xyz.openbmc_project.ObjectMapper",
        path + "/storage", "xyz.openbmc_project.Association", "endpoints",
        [asyncResp, nvmeId](const boost::system::error_code ec,
                            const std::vector<std::string>& storageList) {
        if (ec || storageList.empty())
        {
            BMCWEB_LOG_DEBUG
                << "Failed to find storage that is associated with " << nvmeId;
            return;
        }
        if (storageList.size() > 1)
        {
            BMCWEB_LOG_DEBUG << nvmeId
                             << " is associated with mutliple storages";
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
        [asyncResp, nvmeId](const boost::system::error_code ec,
                            const std::vector<std::string>& controllerList) {
        populateNVMeControllerCollection(asyncResp, nvmeId, ec, controllerList);
        });
}

inline void
    handleGoogleNvme(const crow::Request& /*unused*/,
                     const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& nvme)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp,
         nvme](const boost::system::error_code ec,
               const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            BMCWEB_LOG_INFO << "DBUS error: no matched iface for NVMe: " << ec;
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
    const crow::Request& /*unused*/,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    asyncResp->res.jsonValue["@odata.type"] =
        "#GoogleNvmeCollection.GoogleNvmeCollection";
    asyncResp->res.jsonValue["@odata.id"] = "/google/v1/NVMe";
    asyncResp->res.jsonValue["Name"] = "Google NVMe Collection";
    redfish::collection_util::getCollectionMembers(
        asyncResp, "/google/v1/NVMe",
        {"xyz.openbmc_project.Configuration.NVME1000"});
}

inline void getIdentity(const std::string& controllerNamespace,
                        const std::string& identityId,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
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
        [asyncResp, controllerNamespace, identityId,
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
            [asyncResp, objects,
             controllerPath](const boost::system::error_code ec3,
                             const sdbusplus::message::unix_fd& fd) {
            if (ec3)
            {

                // do not add err msg in redfish response, because this is not
                // mandatory property
                BMCWEB_LOG_INFO
                    << "Failed to call NVMe Identity: " << ec3.message()
                    << "\n";
                redfish::messages::internalError(asyncResp->res);
                return;
            }

            fetchFile(asyncResp, fd);
            },
            objects[0].first, controllerPath,
            "xyz.openbmc_project.Nvme.NVMeAdmin", "Identify",
            controllerNamespace, identityId);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", controllerPath,
        std::array<const char*, 1>{"xyz.openbmc_project.Nvme.NVMeAdmin"});
}

inline void handleGoogleNvmeControllerIdentifyAction(
    const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& nvmeId, const std::string& controllerId)
{
    std::string controllerNamespace;
    std::string idendityId;

    if (!redfish::json_util::readJsonAction(
            req, asyncResp->res, "ControllerNamespace", controllerNamespace,
            "ControllerId", idendityId))
    {
        redfish::messages::actionParameterMissing(
            asyncResp->res, "ControllerNamespace", "ControllerId");
        return;
    }
    handleGoogleNvmeController(
        asyncResp, nvmeId,
        [controllerNamespace, idendityId, asyncResp,
         controllerId](const boost::system::error_code ec,
                       const std::vector<std::string>& controllerList) {
        getIdentity(controllerNamespace, idendityId, asyncResp, controllerId,
                    ec, controllerList);
        });
}

inline void getLogPage(const std::string& logSpecificId,
                       const std::string& logId,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
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
            BMCWEB_LOG_ERROR << "Service supporting " << controllerPath
                             << " is not equal to 1";
            redfish::messages::internalError(asyncResp->res);
            return;
        }

        crow::connections::systemBus->async_method_call(
            [asyncResp, objects,
             controllerPath](const boost::system::error_code ec3,
                             const sdbusplus::message::unix_fd& fd) {
            if (ec3)
            {
                BMCWEB_LOG_INFO << "Failed to call NVMe Log " << ec3 << "\n";

                BMCWEB_LOG_INFO << "Test NVMe Log " << objects[0].first << "\n";
                BMCWEB_LOG_INFO << "Test NVMe Log " << controllerPath << "\n";

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
    const std::string& nvmeId, const std::string& controllerId)
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
        asyncResp, nvmeId,
        [logSpecificId, logId, asyncResp,
         controllerId](const boost::system::error_code ec,
                       const std::vector<std::string>& controllerList) {
        getLogPage(logSpecificId, logId, asyncResp, controllerId, ec,
                   controllerList);
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
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& nvmeId, const std::string& controllerId) {
        handleGoogleNvmeController(
            asyncResp, nvmeId,
            [asyncResp, nvmeId,
             controllerId](const boost::system::error_code ec,
                           const std::vector<std::string>& controllerList) {
            setupNVMeController(asyncResp, nvmeId, controllerId, ec,
                                controllerList);
            });
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
