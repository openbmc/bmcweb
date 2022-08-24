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

static std::optional<boost::asio::posix::stream_descriptor> fileConn;

static void fetchFile(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& output)
{
    if (!fileConn)
    {
        BMCWEB_LOG_ERROR << "Fetch File Context is not setup properly";
        redfish::messages::internalError(asyncResp->res);
        return;
    }

    static std::array<char, 1024> readBuffer;
    fileConn->async_read_some(
        boost::asio::buffer(readBuffer),
        [asyncResp, output](const boost::system::error_code& ec,
                            const std::size_t& bytesTransferred) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "Callback Error: " << ec.message();
            asyncResp->res.addHeader(boost::beast::http::field::content_type,
                                     "application/octet-stream");
            asyncResp->res.body() = output;
            fileConn->close();
            return;
        }
        std::string data(readBuffer.data());
        fetchFile(asyncResp, output + data.substr(0, bytesTransferred));
        });
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
        asyncResp->res.jsonValue["Actions"]["#Nvme.Identify"]["target"] =
            crow::utility::urlFromPieces("google", "v1", "NVMe", nvmeId,
                                         "controllers", controllerId, "Actions",
                                         "NVMe.Identify");

        asyncResp->res.jsonValue["NVMe"]["@odata.id"] =
            crow::utility::urlFromPieces("google", "v1", "NVMe", nvmeId);

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

inline void processDriveResource(const std::optional<std::string>& nvme,
                                 std::function<void(const std::string&)>&& cb,
                                 bool checkId = true)
{
    crow::connections::systemBus->async_method_call(
        [nvme, cb{std::forward<std::function<void(const std::string&)>>(cb)},
         checkId](const boost::system::error_code ec,
                  const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            BMCWEB_LOG_INFO << "DBUS error: no matched iface for Drive: "
                            << ec.message();
            return;
        }

        for (const auto& [path, services] : subtree)
        {
            if (nvme.has_value() &&
                sdbusplus::message::object_path(path).filename() != *nvme)
            {
                continue;
            }

            for (const auto& [service, interfaces] : services)
            {
                for (const std::string& interface : interfaces)
                {
                    if (interface != "xyz.openbmc_project.Inventory.Item.Drive")
                    {
                        continue;
                    }

                    sdbusplus::asio::getProperty<std::string>(
                        *crow::connections::systemBus, service, path, interface,
                        "Protocol",
                        [path, cb](const boost::system::error_code ec2,
                                   const std::string& driveProtocol) {
                        if (ec2)
                        {
                            return;
                        }

                        if (driveProtocol !=
                            "xyz.openbmc_project.Inventory.Item.Drive.DriveProtocol.NVMe")
                        {
                            return;
                        }

                        cb(path);
                        });
                }
            }
        }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0,
        std::array<const char*, 1>{"xyz.openbmc_project.Inventory.Item.Drive"});
}

void handleGoogleNvmeController(const std::string& nvme,
                                NVMeControllerHandlerCb&& cb)
{
    processDriveResource(nvme, [cb{std::forward<NVMeControllerHandlerCb>(cb)}](
                                   const std::string& path) {
        sdbusplus::asio::getProperty<std::vector<std::string>>(
            *crow::connections::systemBus, "xyz.openbmc_project.ObjectMapper",
            path + "/controllers", "xyz.openbmc_project.Association",
            "endpoints", cb);
    });
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
    processDriveResource(nvme, [asyncResp, nvme](const std::string& path) {
        populateNVMe(asyncResp, path, nvme);
    });
}

inline void handleGoogleNvmeCollection(
    const crow::Request& /*unused*/,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    asyncResp->res.jsonValue["@odata.type"] =
        "#GoogleNvmeCollection.GoogleNvmeCollection";
    asyncResp->res.jsonValue["@odata.id"] = "/google/v1/NVMe";
    asyncResp->res.jsonValue["Name"] = "Google NVMe Collection";

    asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
    asyncResp->res.jsonValue["Members@odata.count"] = 0;

    processDriveResource(std::nullopt, [asyncResp](const std::string& path) {
        std::string id = sdbusplus::message::object_path(path).filename();
        if (id.empty())
        {
            return;
        }

        nlohmann::json& members = asyncResp->res.jsonValue["Members"];
        nlohmann::json::object_t member;
        member["@odata.id"] =
            crow::utility::urlFromPieces("google", "v1", "NVMe", id);
        members.push_back(std::move(member));
        asyncResp->res.jsonValue["Members@odata.count"] = members.size();
    });
}

inline void getIdentity(const std::string& controllerNamespace,
                        const std::string& controllerNamespaceId,
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
        [asyncResp, controllerNamespace, controllerNamespaceId, identityId,
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

        if (fileConn->is_open())
        {
            BMCWEB_LOG_ERROR << "Fetch File Context is currently being used";
            redfish::messages::internalError(asyncResp->res);
            return;
        }

        crow::connections::systemBus->async_method_call(
            [asyncResp, objects,
             controllerPath](const boost::system::error_code ec3,
                             const sdbusplus::message::unix_fd& fd) {
            if (ec3)
            {
                BMCWEB_LOG_INFO << "Failed to call NVMe Identity: "
                                << ec3.message();
                redfish::messages::internalError(asyncResp->res);
                return;
            }

            int dupFd = dup(fd.fd);
            fileConn->assign(dupFd);
            fetchFile(asyncResp, "");
            },
            objects[0].first, controllerPath,
            "xyz.openbmc_project.Nvme.NVMeAdmin", "Identify",
            controllerNamespace, controllerNamespaceId, identityId);
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
    std::string controllerNamespaceId;
    std::string idendityId;

    if (!redfish::json_util::readJsonAction(
            req, asyncResp->res, "CNS", controllerNamespace, "NSID",
            controllerNamespaceId, "CNTID", idendityId))
    {
        redfish::messages::actionParameterMissing(
            asyncResp->res, "ControllerNamespace", "ControllerNamespaceId");
        return;
    }
    handleGoogleNvmeController(
        nvmeId,
        [controllerNamespace, controllerNamespaceId, idendityId, asyncResp,
         controllerId](const boost::system::error_code ec,
                       const std::vector<std::string>& controllerList) {
        getIdentity(controllerNamespace, controllerNamespaceId, idendityId,
                    asyncResp, controllerId, ec, controllerList);
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

        if (fileConn->is_open())
        {
            BMCWEB_LOG_ERROR << "Fetch File Context is currently being used";
            redfish::messages::internalError(asyncResp->res);
            return;
        }

        crow::connections::systemBus->async_method_call(
            [asyncResp, objects,
             controllerPath](const boost::system::error_code ec3,
                             const sdbusplus::message::unix_fd& fd) {
            if (ec3)
            {
                BMCWEB_LOG_INFO << "Failed to call NVMe Log " << ec3.message();
                redfish::messages::internalError(asyncResp->res);
                return;
            }

            int dupFd = dup(fd.fd);
            fileConn->assign(dupFd);
            fetchFile(asyncResp, "");
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
    std::string logSpecificField;
    std::string logId;

    if (!redfish::json_util::readJsonAction(req, asyncResp->res, "LID",
                                            logSpecificId, "LSP", logSpecificId,
                                            "LSI", logId))
    {
        redfish::messages::actionParameterMissing(asyncResp->res,
                                                  "LogSpecificId", "LogId");
        return;
    }

    handleGoogleNvmeController(
        nvmeId, [logSpecificId, logId, asyncResp,
                 controllerId](const boost::system::error_code ec,
                               const std::vector<std::string>& controllerList) {
            getLogPage(logSpecificId, logId, asyncResp, controllerId, ec,
                       controllerList);
        });
}

inline void setupGoogleNVMeActionFileConn(boost::asio::io_context& ioc)
{
    fileConn.emplace(ioc);
}

inline void requestGoogleNVMeControllerActionIdentify(App& app)
{
    BMCWEB_ROUTE(
        app, "/google/v1/NVMe/<str>/controllers/<str>/Actions/NVMe.Identify")
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& nvmeId, const std::string& controllerId) {
        handleGoogleNvmeControllerIdentifyAction(req, asyncResp, nvmeId,
                                                 controllerId);
        });
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
            nvmeId, [asyncResp, nvmeId, controllerId](
                        const boost::system::error_code ec,
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
