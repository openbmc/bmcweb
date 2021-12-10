
#pragma once

#include "../../include/dbus_utility.hpp"
#include "../../include/openbmc_dbus_rest.hpp"

#include <app.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <sdbusplus/asio/property.hpp>

#include <string>
#include <string_view>

namespace redfish
{

using SensorVariantType = dbus::utility::DbusVariantType;

std::optional<std::pair<std::string, std::string>>
    checkAndGetSoleGetSubTreeResponse(
        const crow::openbmc_mapper::GetSubTreeType& subtree)
{
    std::optional<std::pair<std::string, std::string>> ret;

    if (subtree.empty())
    {
        BMCWEB_LOG_DEBUG << "Can't find bmc D-Bus object!";
        return {};
    }

    if (subtree.size() > 1)
    {
        BMCWEB_LOG_DEBUG << "Found more than 1 bmc D-Bus object!";
        return {};
    }

    // Assume only 1 bmc D-Bus object
    // Throw an error if there is more than 1
    if (subtree.size() > 1)
    {
        BMCWEB_LOG_DEBUG << "Found more than 1 bmc D-Bus object!";
        return {};
    }

    if (subtree[0].first.empty() || subtree[0].second.size() != 1)
    {
        BMCWEB_LOG_DEBUG << "Error getting bmc D-Bus object!";
        return {};
    }

    return std::make_pair(subtree[0].first, subtree[0].second[0].first);
}

// Returns the sole non-Object Mapper service
std::optional<std::string>
    checkAndGetSoleNonMapperGetObjectResponse(const GetObjectType& objects)
{
    std::optional<std::pair<std::string, std::string>> ret;

    if (objects.empty())
    {
        return {};
    }

    std::optional<std::string> service;
    for (const std::pair<std::string, std::vector<std::string>>&
             serviceAndIfaces : objects)
    {
        const std::string& s = serviceAndIfaces.first;
        if (s != "xyz.openbmc_project.ObjectMapper")
        {
            if (service.has_value())
            { // Already found one
                return {};
            }
            service = s;
        }
    }

    return service;
}

/**
 * @brief Checks whether the last segment of a DBus object is a specific value.
 *
 * @param[i] objectPath - the DBus object path to check
 *
 * @return bool
 */
bool dbusObjectNameLastSegmentEquals(const std::string& objectName,
                                     const std::string& value)
{
    size_t idx = objectName.rfind('/');
    if (idx == std::string::npos)
    {
        return false;
    }
    return (objectName.substr(idx + 1) == value);
}

/**
 * @brief Populates on entry in ManagerDiagnosticData with one utility sensor
 * DBus object.
 *
 * @param[i,o] asyncResp - Async response object
 * @param[i] diagnosticDataObjectPath - the DBus object path of the utility
 * sensor.
 *
 * @return void
 */
inline void populateOneManagerDiagnosticDataEntry(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& diagnosticDataObjectPath)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, diagnosticDataObjectPath](
            const boost::system::error_code ec, const GetObjectType& objects) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Error occurred GetObject";
                return;
            }

            std::optional<std::string> serviceName =
                checkAndGetSoleNonMapperGetObjectResponse(objects);
            if (!serviceName.has_value())
            {
                BMCWEB_LOG_ERROR << "Cannot get service from object name"
                                 << diagnosticDataObjectPath;
                return;
            }

            sdbusplus::asio::getProperty<double>(
                *crow::connections::systemBus, serviceName.value(),
                diagnosticDataObjectPath, "xyz.openbmc_project.Sensor.Value",
                "Value",
                [asyncResp, diagnosticDataObjectPath](
                    const boost::system::error_code ec, const double v) {
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG
                            << "DBus response error on getProperty " << ec;
                        return;
                    }
                    if (dbusObjectNameLastSegmentEquals(
                            diagnosticDataObjectPath, "CPU_Kernel"))
                    {
                        asyncResp->res
                            .jsonValue["ProcessorStatistics"]["KernelPercent"] =
                            v;
                    }
                    else if (dbusObjectNameLastSegmentEquals(
                                 diagnosticDataObjectPath, "CPU_User"))
                    {
                        asyncResp->res
                            .jsonValue["ProcessorStatistics"]["UserPercent"] =
                            v;
                    }
                    else if (dbusObjectNameLastSegmentEquals(
                                 diagnosticDataObjectPath,
                                 "Memory_AvailableBytes"))
                    {
                        asyncResp->res
                            .jsonValue["MemoryStatistics"]["AvailableBytes"] =
                            v;
                    }
                });
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject",
        diagnosticDataObjectPath, std::array<const char*, 0>{});
}

/**
 * @brief Find the endpoints for an Association definition.
 *
 * @param[i,o] asyncResp - Async response object
 * @param[i] bmcAssociationObjectPath - the DBus object path of the Association
 * definition.
 * @param[i] bmcAssociationService - the DBus service that contains the
 * Association definition.
 *
 * @return void
 */
inline void findBmcAssociationEndpoints(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& bmcAssociationObjectPath,
    const std::string& bmcAssociationService)
{
    // No need to use std::variant when using getProperty, just use the
    // contained type in the template parameter
    sdbusplus::asio::getProperty<std::vector<std::string>>(
        *crow::connections::systemBus, bmcAssociationService,
        bmcAssociationObjectPath, "xyz.openbmc_project.Association",
        "endpoints",
        [asyncResp](const boost::system::error_code ec,
                    const std::vector<std::string>& v) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBus error getting association endpoints";
                return;
            }

            for (const std::string& endpoint : v)
            {
                populateOneManagerDiagnosticDataEntry(asyncResp, endpoint);
            }
        });
}

/**
 * @brief Find Associations related to one BMC inventory item.
 *
 * @param[i,o] asyncResp - Async response object
 * @param[i] bmcObjectPath - the DBus object path of the BMC inventory item.
 *
 * @return void
 */
inline void
    findBmcAssociation(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& bmcObjectPath)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp,
         bmcObjectPath](const boost::system::error_code ec,
                        const crow::openbmc_mapper::GetSubTreeType& subtree) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBus error getting associations for BMC "
                                    "inventory object "
                                 << ec;
                return;
            }

            // The expected values may be as follows, depending on whether the
            // static/dynamic stack is used. subtree[0].first:
            // "/xyz/openbmc_project/inventory/system/bmc/bmc/bmc_diagnostic_data"
            // subtree[0].second[0]: "xyz.openbmc_project.ObjectMapper"
            std::optional<std::pair<std::string, std::string>> pathAndService =
                checkAndGetSoleGetSubTreeResponse(subtree);
            if (!pathAndService.has_value())
            {
                return;
            }

            findBmcAssociationEndpoints(asyncResp, pathAndService.value().first,
                                        pathAndService.value().second);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", bmcObjectPath,
        int32_t(0),
        std::array<const char*, 1>{"xyz.openbmc_project.Association"});
}

/**
 * @brief Find BMC inventories in the system and iterate through them.
 *
 * @param[i,o] asyncResp - Async response object
 *
 * @return void
 */
inline void
    findBmcInventory(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec,
                    const crow::openbmc_mapper::GetSubTreeType& subtree) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBus response error on GetSubTree " << ec;
                return;
            }

            // For Entity-Manager based systems ("dynamic inventory stack"):
            // subtree[0].first           may be
            // "/xyz/openbmc_project/inventory/system/bmc/bmc".
            // subtree[0].second[0].first may be {
            // "xyz.openbmc_project.EntityManager",
            //   {"xyz.openbmc_project.AddObject"
            //   "xyz.openbmc_project.Inventory.Item.Bmc"} }.

            // For Inventory-Manager based systems ("static inventory stack"):
            // subtree[0].first           may be
            // "/xyz/openbmc_project/inventory/bmc", depending on compile-time
            // configuration subtree[0].second[0].first may be {
            // ""xyz.openbmc_project.Inventory.Manager",
            //   {"org.freedesktop.DBus.Introspectable",
            //    "org.freedesktop.DBus.Peer",
            //    "org.freedesktop.DBus.Properties",
            //    "xyz.openbmc_project.Inventory.Item.Bmc"} }
            std::optional<std::pair<std::string, std::string>> pathAndService =
                checkAndGetSoleGetSubTreeResponse(subtree);
            if (!pathAndService.has_value())
            {
                return;
            }

            // Found BMC object path. Ask ObjectMapper about the reverse
            // Association edges.
            findBmcAssociation(asyncResp, pathAndService.value().first);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", int32_t(0),
        std::array<const char*, 1>{"xyz.openbmc_project.Inventory.Item.Bmc"});
}

inline void requestRoutesManagerDiagnosticData(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/ManagerDiagnosticData")
        .privileges(redfish::privileges::getManagerDiagnosticData)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request& /*req*/,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#ManagerDiagnosticData.v1_0_0.ManagerDiagnosticData";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Managers/bmc/ManagerDiagnosticData";
                const std::string& idAndName = "BMC's ManagerDiagnosticData";
                asyncResp->res.jsonValue["Id"] = idAndName;
                asyncResp->res.jsonValue["Name"] = idAndName;

                // Start the first step of the 4-step process to populate
                // the BMC diagnostic data
                findBmcInventory(asyncResp);
            });
}

} // namespace redfish
