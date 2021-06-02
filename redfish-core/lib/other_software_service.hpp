
#pragma once

#include "bmcweb_config.h"

#include "dbus_singleton.hpp"
#include "error_messages.hpp"

// TODO(wltu): Move to a different file until this is fully cleaned up.
#include <sdbusplus/asio/property.hpp>

namespace redfish
{
inline static void
    getRelatedItemsDrive(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                         const sdbusplus::message::object_path& objPath)
{
    // Drive is expected to be under a Chassis
    sdbusplus::asio::getProperty<std::vector<std::string>>(
        *crow::connections::systemBus, "xyz.openbmc_project.ObjectMapper",
        objPath.str + "/Storage", "xyz.openbmc_project.Association",
        "endpoints",
        [aResp, objPath](const boost::system::error_code ec,
                         const std::vector<std::string>& storageList) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG
                    << "Failed to call getProperty for Drive -> Storage Association: "
                    << ec;
                return;
            }

            if (storageList.size() != 1)
            {
                BMCWEB_LOG_ERROR
                    << "Resource can only be included in one storage";
                messages::internalError(aResp->res);
                return;
            }

            nlohmann::json& relatedItem = aResp->res.jsonValue["RelatedItem"];
            relatedItem.push_back(
                {{"@odata.id",
                  "/redfish/v1/Systems/system/Storage/" +
                      sdbusplus::message::object_path(storageList[0])
                          .filename() +
                      "/Drives/" + objPath.filename()}});
            aResp->res.jsonValue["RelatedItem@odata.count"] =
                relatedItem.size();
        });
}

inline static void getRelatedItemsStorageController(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const sdbusplus::message::object_path& objPath)
{
    sdbusplus::asio::getProperty<std::vector<std::string>>(
        *crow::connections::systemBus, "xyz.openbmc_project.ObjectMapper",
        objPath.str + "/Storage", "xyz.openbmc_project.Association",
        "endpoints",
        [aResp, objPath](const boost::system::error_code ec,
                         const std::vector<std::string>& storageList) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG
                    << "Failed to call getProperty for StorageController -> Storage Association: "
                    << ec;
                return;
            }

            if (storageList.size() != 1)
            {
                BMCWEB_LOG_ERROR
                    << "Resource can only be included in one storage";
                messages::internalError(aResp->res);
                return;
            }

            sdbusplus::message::object_path storage(storageList[0]);

            sdbusplus::asio::getProperty<std::vector<std::string>>(
                *crow::connections::systemBus,
                "xyz.openbmc_project.ObjectMapper",
                storage.str + "/StorageController",
                "xyz.openbmc_project.Association", "endpoints",
                [aResp, storage, objPath](
                    const boost::system::error_code ec,
                    const std::vector<std::string>& storageControllerList) {
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG
                            << "Failed to call getProperty for Storage -> StorageController Association: "
                            << ec;
                        return;
                    }

                    nlohmann::json& relatedItem =
                        aResp->res.jsonValue["RelatedItem"];
                    nlohmann::json& relatedItemCount =
                        aResp->res.jsonValue["RelatedItem@odata.count"];

                    size_t index = 0;
                    for (size_t i = 0; i < storageControllerList.size(); ++i)
                    {
                        const std::string& storageController =
                            storageControllerList[i];
                        const std::string& id =
                            sdbusplus::message::object_path(storageController)
                                .filename();
                        if (id.empty())
                        {
                            BMCWEB_LOG_ERROR << "filename() is empty in "
                                             << storageController;
                            continue;
                        }

                        ++index;

                        if (storageControllerList[i] != objPath.str)
                        {
                            continue;
                        }
                        relatedItem.push_back(
                            {{"@odata.id",
                              "/redfish/v1/Systems/system/Storage/" +
                                  storage.filename() + "#/StorageControllers/" +
                                  std::to_string(i)}});

                        break;
                    }
                    relatedItemCount = relatedItem.size();
                });
        });
}

inline static void
    getRelatedItemsOther(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                         const sdbusplus::message::object_path& association)
{

    // Find supported device types.
    crow::connections::systemBus->async_method_call(
        [aResp, association](
            const boost::system::error_code ec,
            const std::vector<std::pair<std::string, std::vector<std::string>>>&
                objects) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "error_code = " << ec
                                 << ", error msg = " << ec.message();
                return;
            }
            if (objects.empty())
            {
                return;
            }

            nlohmann::json& relatedItem = aResp->res.jsonValue["RelatedItem"];
            nlohmann::json& relatedItemCount =
                aResp->res.jsonValue["RelatedItem@odata.count"];

            for (const std::pair<std::string, std::vector<std::string>>&
                     object : objects)
            {
                for (const std::string& interfaces : object.second)
                {
                    if (interfaces ==
                        "xyz.openbmc_project.Inventory.Item.Drive")
                    {
                        getRelatedItemsDrive(aResp, association);
                    }

                    if (interfaces ==
                            "xyz.openbmc_project.Inventory.Item.Accelerator" ||
                        interfaces == "xyz.openbmc_project.Inventory.Item.Cpu")
                    {
                        relatedItem.push_back(
                            {{"@odata.id",
                              "/redfish/v1/Systems/system/Processors/" +
                                  association.filename()}});
                    }

                    if (interfaces ==
                            "xyz.openbmc_project.Inventory.Item.Board" ||
                        interfaces ==
                            "xyz.openbmc_project.Inventory.Item.Chassis")
                    {
                        relatedItem.push_back(
                            {{"@odata.id", "/redfish/v1/Chassis/" +
                                               association.filename()}});
                    }

                    if (interfaces == "xyz.openbmc_project.Inventory."
                                      "Item.StorageController")
                    {
                        getRelatedItemsStorageController(aResp, association);
                    }
                }
            }

            relatedItemCount = relatedItem.size();
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", association.str,
        std::array<const char*, 6>{
            "xyz.openbmc_project.Inventory.Item.Accelerator",
            "xyz.openbmc_project.Inventory.Item.Cpu",
            "xyz.openbmc_project.Inventory.Item.Drive",
            "xyz.openbmc_project.Inventory.Item.Board",
            "xyz.openbmc_project.Inventory.Item.Chassis",
            "xyz.openbmc_project.Inventory.Item.StorageController"});
}

/*
    Fill related item links for Software with other purposes.
    Use other purpose for device level softwares.
*/
inline static void
    getRelatedItemsOthers(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                          std::string_view path)
{
    BMCWEB_LOG_DEBUG << "getRelatedItemsOthers enter";

    aResp->res.jsonValue["RelatedItem"] = nlohmann::json::array();
    aResp->res.jsonValue["RelatedItem@odata.count"] = 0;

    BMCWEB_LOG_DEBUG << "GetReltatedItemsOthers for " << std::string(path)
                     << "/SoftwareRelated";

    sdbusplus::asio::getProperty<std::vector<std::string>>(
        *crow::connections::systemBus, "xyz.openbmc_project.ObjectMapper",
        std::string(path) + "/SoftwareRelated",
        "xyz.openbmc_project.Association", "endpoints",
        [aResp](const boost::system::error_code ec,
                const std::vector<std::string>& relatedList) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "Failed to call dbus for getLogEntry";
                return;
            }

            for (const auto& item : relatedList)
            {
                sdbusplus::message::object_path itemPath(item);
                if (itemPath.filename().empty())
                {
                    continue;
                }

                getRelatedItemsOther(aResp, itemPath);
            }
        });
}
} // namespace redfish