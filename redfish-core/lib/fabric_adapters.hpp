#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "dbus_utility.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"

#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace redfish
{

inline void handleAdapterError(const boost::system::error_code& ec,
                               crow::Response& res,
                               const std::string& adapterId)
{
    if (ec.value() == boost::system::errc::io_error)
    {
        messages::resourceNotFound(res, "#FabricAdapter.v1_4_0.FabricAdapter",
                                   adapterId);
        return;
    }

    BMCWEB_LOG_ERROR("DBus method call failed with error {}", ec.value());
    messages::internalError(res);
}

inline void getFabricAdapterLocation(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& serviceName, const std::string& fabricAdapterPath)
{
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, serviceName, fabricAdapterPath,
        "xyz.openbmc_project.Inventory.Decorator.LocationCode", "LocationCode",
        [asyncResp](const boost::system::error_code& ec,
                    const std::string& property) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR("DBUS response error for Location");
                messages::internalError(asyncResp->res);
            }
            return;
        }

        asyncResp->res.jsonValue["Location"]["PartLocation"]["ServiceLabel"] =
            property;
    });
}

inline void
    getFabricAdapterAsset(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& serviceName,
                          const std::string& fabricAdapterPath)
{
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, serviceName, fabricAdapterPath,
        "xyz.openbmc_project.Inventory.Decorator.Asset",
        [fabricAdapterPath, asyncResp{asyncResp}](
            const boost::system::error_code& ec,
            const dbus::utility::DBusPropertiesMap& propertiesList) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR("DBUS response error for Properties");
                messages::internalError(asyncResp->res);
            }
            return;
        }

        const std::string* serialNumber = nullptr;
        const std::string* model = nullptr;
        const std::string* partNumber = nullptr;
        const std::string* sparePartNumber = nullptr;

        const bool success = sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), propertiesList, "SerialNumber",
            serialNumber, "Model", model, "PartNumber", partNumber,
            "SparePartNumber", sparePartNumber);

        if (!success)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        if (serialNumber != nullptr)
        {
            asyncResp->res.jsonValue["SerialNumber"] = *serialNumber;
        }

        if (model != nullptr)
        {
            asyncResp->res.jsonValue["Model"] = *model;
        }

        if (partNumber != nullptr)
        {
            asyncResp->res.jsonValue["PartNumber"] = *partNumber;
        }

        if (sparePartNumber != nullptr && !sparePartNumber->empty())
        {
            asyncResp->res.jsonValue["SparePartNumber"] = *sparePartNumber;
        }
    });
}

inline void
    getFabricAdapterState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& serviceName,
                          const std::string& fabricAdapterPath)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, serviceName, fabricAdapterPath,
        "xyz.openbmc_project.Inventory.Item", "Present",
        [asyncResp](const boost::system::error_code& ec, const bool present) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR("DBUS response error for State");
                messages::internalError(asyncResp->res);
            }
            return;
        }

        if (!present)
        {
            asyncResp->res.jsonValue["Status"]["State"] = "Absent";
        }
    });
}

inline void
    getFabricAdapterHealth(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& serviceName,
                           const std::string& fabricAdapterPath)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, serviceName, fabricAdapterPath,
        "xyz.openbmc_project.State.Decorator.OperationalStatus", "Functional",
        [asyncResp](const boost::system::error_code& ec,
                    const bool functional) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR("DBUS response error for Health");
                messages::internalError(asyncResp->res);
            }
            return;
        }

        if (!functional)
        {
            asyncResp->res.jsonValue["Status"]["Health"] = "Critical";
        }
    });
}

inline void doAdapterGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& systemName,
                         const std::string& adapterId,
                         const std::string& fabricAdapterPath,
                         const std::string& serviceName)
{
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/FabricAdapter/FabricAdapter.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] =
        "#FabricAdapter.v1_4_0.FabricAdapter";
    asyncResp->res.jsonValue["Name"] = "Fabric Adapter";
    asyncResp->res.jsonValue["Id"] = adapterId;
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/FabricAdapters/{}", systemName, adapterId);

    asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
    asyncResp->res.jsonValue["Status"]["Health"] = "OK";

    asyncResp->res.jsonValue["Ports"]["@odata.id"] =
        boost::urls::format("/redfish/v1/Systems/{}/FabricAdapters/{}/Ports",
                            systemName, adapterId);

    getFabricAdapterLocation(asyncResp, serviceName, fabricAdapterPath);
    getFabricAdapterAsset(asyncResp, serviceName, fabricAdapterPath);
    getFabricAdapterState(asyncResp, serviceName, fabricAdapterPath);
    getFabricAdapterHealth(asyncResp, serviceName, fabricAdapterPath);
}

inline bool checkFabricAdapterId(const std::string& adapterPath,
                                 const std::string& adapterId)
{
    std::string fabricAdapterName =
        sdbusplus::message::object_path(adapterPath).filename();

    return !(fabricAdapterName.empty() || fabricAdapterName != adapterId);
}

inline void getValidFabricAdapterPath(
    const std::string& adapterId, const std::string& systemName,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::function<void(const std::string& fabricAdapterPath,
                       const std::string& serviceName)>&& callback)
{
    if (systemName != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    constexpr std::array<std::string_view, 1> interfaces{
        "xyz.openbmc_project.Inventory.Item.FabricAdapter"};

    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [adapterId, asyncResp,
         callback](const boost::system::error_code& ec,
                   const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            handleAdapterError(ec, asyncResp->res, adapterId);
            return;
        }
        for (const auto& [adapterPath, serviceMap] : subtree)
        {
            if (checkFabricAdapterId(adapterPath, adapterId))
            {
                callback(adapterPath, serviceMap.begin()->first);
                return;
            }
        }
        BMCWEB_LOG_WARNING("Adapter not found");
        messages::resourceNotFound(asyncResp->res, "FabricAdapter", adapterId);
    });
}

inline void
    handleFabricAdapterGet(App& app, const crow::Request& req,
                           const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& systemName,
                           const std::string& adapterId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (bmcwebEnableMultiHost)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    getValidFabricAdapterPath(
        adapterId, systemName, asyncResp,
        [asyncResp, systemName, adapterId](const std::string& fabricAdapterPath,
                                           const std::string& serviceName) {
        doAdapterGet(asyncResp, systemName, adapterId, fabricAdapterPath,
                     serviceName);
    });
}

inline void handleFabricAdapterCollectionGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (bmcwebEnableMultiHost)
    {
        // Option currently returns no systems. TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/FabricAdapterCollection/FabricAdapterCollection.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] =
        "#FabricAdapterCollection.FabricAdapterCollection";
    asyncResp->res.jsonValue["Name"] = "Fabric Adapter Collection";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/FabricAdapters", systemName);

    constexpr std::array<std::string_view, 1> interfaces{
        "xyz.openbmc_project.Inventory.Item.FabricAdapter"};
    collection_util::getCollectionMembers(
        asyncResp,
        boost::urls::url("/redfish/v1/Systems/system/FabricAdapters"),
        interfaces, "/xyz/openbmc_project/inventory");
}

inline void handleFabricAdapterCollectionHead(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (bmcwebEnableMultiHost)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/FabricAdapterCollection/FabricAdapterCollection.json>; rel=describedby");
}

inline void
    handleFabricAdapterHead(crow::App& app, const crow::Request& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& systemName,
                            const std::string& adapterId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if constexpr (bmcwebEnableMultiHost)
    {
        // Option currently returns no systems. TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    getValidFabricAdapterPath(adapterId, systemName, asyncResp,
                              [asyncResp, systemName, adapterId](
                                  const std::string&, const std::string&) {
        asyncResp->res.addHeader(
            boost::beast::http::field::link,
            "</redfish/v1/JsonSchemas/FabricAdapter/FabricAdapter.json>; rel=describedby");
    });
}

inline void requestRoutesFabricAdapterCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/FabricAdapters/")
        .privileges(redfish::privileges::getFabricAdapterCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFabricAdapterCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/FabricAdapters/")
        .privileges(redfish::privileges::headFabricAdapterCollection)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleFabricAdapterCollectionHead, std::ref(app)));
}

inline void requestRoutesFabricAdapters(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/FabricAdapters/<str>/")
        .privileges(redfish::privileges::getFabricAdapter)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFabricAdapterGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/FabricAdapters/<str>/")
        .privileges(redfish::privileges::headFabricAdapter)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleFabricAdapterHead, std::ref(app)));
}
} // namespace redfish
