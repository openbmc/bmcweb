#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"

#include <boost/system/error_code.hpp>
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

    BMCWEB_LOG_ERROR << "DBus method call failed with error " << ec.value();
    messages::internalError(res);
}

inline void
    getFabricAdapterLocation(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                             const std::string& serviceName,
                             const std::string& fabricAdapterPath)
{
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, serviceName, fabricAdapterPath,
        "xyz.openbmc_project.Inventory.Decorator.LocationCode", "LocationCode",
        [aResp](const boost::system::error_code& ec,
                const std::string& property) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR << "DBUS response error for Location";
                messages::internalError(aResp->res);
            }
            return;
        }

        aResp->res.jsonValue["Location"]["PartLocation"]["ServiceLabel"] =
            property;
        });
}

inline void
    getFabricAdapterAsset(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                          const std::string& serviceName,
                          const std::string& fabricAdapterPath)
{
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, serviceName, fabricAdapterPath,
        "xyz.openbmc_project.Inventory.Decorator.Asset",
        [fabricAdapterPath,
         aResp{aResp}](const boost::system::error_code& ec,
                       const dbus::utility::DBusPropertiesMap& propertiesList) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR << "DBUS response error for Properties";
                messages::internalError(aResp->res);
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
            messages::internalError(aResp->res);
            return;
        }

        if (serialNumber != nullptr)
        {
            aResp->res.jsonValue["SerialNumber"] = *serialNumber;
        }

        if (model != nullptr)
        {
            aResp->res.jsonValue["Model"] = *model;
        }

        if (partNumber != nullptr)
        {
            aResp->res.jsonValue["PartNumber"] = *partNumber;
        }

        if (sparePartNumber != nullptr && !sparePartNumber->empty())
        {
            aResp->res.jsonValue["SparePartNumber"] = *sparePartNumber;
        }
        });
}

inline void
    getFabricAdapterState(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                          const std::string& serviceName,
                          const std::string& fabricAdapterPath)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, serviceName, fabricAdapterPath,
        "xyz.openbmc_project.Inventory.Item", "Present",
        [aResp](const boost::system::error_code& ec, const bool present) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR << "DBUS response error for State";
                messages::internalError(aResp->res);
            }
            return;
        }

        if (!present)
        {
            aResp->res.jsonValue["Status"]["State"] = "Absent";
        }
        });
}

inline void
    getFabricAdapterHealth(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                           const std::string& serviceName,
                           const std::string& fabricAdapterPath)
{
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, serviceName, fabricAdapterPath,
        "xyz.openbmc_project.State.Decorator.OperationalStatus", "Functional",
        [aResp](const boost::system::error_code& ec, const bool functional) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR << "DBUS response error for Health";
                messages::internalError(aResp->res);
            }
            return;
        }

        if (!functional)
        {
            aResp->res.jsonValue["Status"]["Health"] = "Critical";
        }
        });
}

inline void doAdapterGet(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                         const std::string& systemName,
                         const std::string& adapterId,
                         const std::string& fabricAdapterPath,
                         const std::string& serviceName)
{
    aResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/FabricAdapter/FabricAdapter.json>; rel=describedby");
    aResp->res.jsonValue["@odata.type"] = "#FabricAdapter.v1_4_0.FabricAdapter";
    aResp->res.jsonValue["Name"] = "Fabric Adapter";
    aResp->res.jsonValue["Id"] = adapterId;
    aResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "Systems", systemName, "FabricAdapters", adapterId);

    aResp->res.jsonValue["Status"]["State"] = "Enabled";
    aResp->res.jsonValue["Status"]["Health"] = "OK";

    getFabricAdapterLocation(aResp, serviceName, fabricAdapterPath);
    getFabricAdapterAsset(aResp, serviceName, fabricAdapterPath);
    getFabricAdapterState(aResp, serviceName, fabricAdapterPath);
    getFabricAdapterHealth(aResp, serviceName, fabricAdapterPath);
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
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    std::function<void(const std::string& fabricAdapterPath,
                       const std::string& serviceName)>&& callback)
{
    if (systemName != "system")
    {
        messages::resourceNotFound(aResp->res, "ComputerSystem", systemName);
        return;
    }
    constexpr std::array<std::string_view, 1> interfaces{
        "xyz.openbmc_project.Inventory.Item.FabricAdapter"};

    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [adapterId, aResp,
         callback](const boost::system::error_code& ec,
                   const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            handleAdapterError(ec, aResp->res, adapterId);
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
        BMCWEB_LOG_WARNING << "Adapter not found";
        messages::resourceNotFound(aResp->res, "FabricAdapter", adapterId);
        });
}

inline void
    handleFabricAdapterGet(App& app, const crow::Request& req,
                           const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                           const std::string& systemName,
                           const std::string& adapterId)
{
    if (!redfish::setUpRedfishRoute(app, req, aResp))
    {
        return;
    }

    getValidFabricAdapterPath(
        adapterId, systemName, aResp,
        [aResp, systemName, adapterId](const std::string& fabricAdapterPath,
                                       const std::string& serviceName) {
        doAdapterGet(aResp, systemName, adapterId, fabricAdapterPath,
                     serviceName);
        });
}

inline void handleFabricAdapterCollectionGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, aResp))
    {
        return;
    }
    if (systemName != "system")
    {
        messages::resourceNotFound(aResp->res, "ComputerSystem", systemName);
        return;
    }

    aResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/FabricAdapterCollection/FabricAdapterCollection.json>; rel=describedby");
    aResp->res.jsonValue["@odata.type"] =
        "#FabricAdapterCollection.FabricAdapterCollection";
    aResp->res.jsonValue["Name"] = "Fabric Adapter Collection";
    aResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "Systems", systemName, "FabricAdapters");

    constexpr std::array<std::string_view, 1> interfaces{
        "xyz.openbmc_project.Inventory.Item.FabricAdapter"};
    collection_util::getCollectionMembers(
        aResp, boost::urls::url("/redfish/v1/Systems/system/FabricAdapters"),
        interfaces);
}

inline void handleFabricAdapterCollectionHead(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, aResp))
    {
        return;
    }
    if (systemName != "system")
    {
        messages::resourceNotFound(aResp->res, "ComputerSystem", systemName);
        return;
    }
    aResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/FabricAdapterCollection/FabricAdapterCollection.json>; rel=describedby");
}

inline void
    handleFabricAdapterHead(crow::App& app, const crow::Request& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                            const std::string& systemName,
                            const std::string& adapterId)
{
    if (!redfish::setUpRedfishRoute(app, req, aResp))
    {
        return;
    }

    getValidFabricAdapterPath(
        adapterId, systemName, aResp,
        [aResp, systemName, adapterId](const std::string&, const std::string&) {
        aResp->res.addHeader(
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
