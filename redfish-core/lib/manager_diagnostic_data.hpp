
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "http_request.hpp"
#include "openbmc_dbus_rest.hpp"
#include "privileges.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "routing.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/asio/property.hpp>

#include <string>
#include <string_view>

namespace redfish
{

inline void
    afterGetManagerStartTime(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                             const boost::system::error_code& ec,
                             uint64_t bmcwebResetTime)
{
    if (ec)
    {
        // Not all servers will be running in systemd, so ignore the error.
        return;
    }
    using std::chrono::steady_clock;

    std::chrono::duration<steady_clock::rep, std::micro> usReset{
        bmcwebResetTime};
    steady_clock::time_point resetTime{usReset};

    steady_clock::time_point now = steady_clock::now();

    steady_clock::duration runTime = now - resetTime;

    if (runTime < steady_clock::duration::zero())
    {
        BMCWEB_LOG_CRITICAL << "Uptime was negative????";
        messages::internalError(aResp->res);
        return;
    }

    // Floor to the closest millisecond
    using Milli = std::chrono::duration<steady_clock::rep, std::milli>;
    Milli milli = std::chrono::floor<Milli>(runTime);

    using SecondsFloat = std::chrono::duration<double>;
    SecondsFloat sec = std::chrono::duration_cast<SecondsFloat>(milli);

    aResp->res.jsonValue["ServiceRootUptimeSeconds"] = sec.count();
}

inline void
    managerGetServiceRootUptime(const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    sdbusplus::asio::getProperty<uint64_t>(
        *crow::connections::systemBus, "org.freedesktop.systemd1",
        "/org/freedesktop/systemd1/unit/bmcweb_2eservice",
        "org.freedesktop.systemd1.Unit", "ActiveEnterTimestampMonotonic",
        std::bind_front(afterGetManagerStartTime, aResp));
}
using SensorVariantType = dbus::utility::DbusVariantType;

std::optional<std::pair<std::string, std::string>>
    checkAndGetSoleGetSubTreeResponse(
        const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (subtree.empty())
    {
        BMCWEB_LOG_DEBUG << "Can't find bmc D-Bus object!";
        return std::nullopt;
    }

    // TODO: Consider Multi-BMC cases
    // Assume only 1 bmc D-Bus object
    // Throw an error if there is more than 1
    if (subtree.size() > 1)
    {
        BMCWEB_LOG_DEBUG << "Found more than 1 bmc D-Bus object!";
        return std::nullopt;
    }

    if (subtree[0].first.empty() || subtree[0].second.size() != 1)
    {
        BMCWEB_LOG_DEBUG << "Error getting bmc D-Bus object!";
        return std::nullopt;
    }

    return std::make_pair(subtree[0].first, subtree[0].second[0].first);
}

// Returns the sole non-Object Mapper service
std::optional<std::string> checkAndGetSoleNonMapperGetObjectResponse(
    const dbus::utility::MapperGetObject& objects)
{
    if (objects.empty())
    {
        return std::nullopt;
    }

    for (const std::pair<std::string, std::vector<std::string>>&
             serviceAndIfaces : objects)
    {
        if (serviceAndIfaces.first != "xyz.openbmc_project.ObjectMapper")
        {
            return serviceAndIfaces.first;
        }
    }
    return std::nullopt;
}

/**
 * @brief Checks whether the last segment of a DBus object is a specific value.
 *
 * @param[i] objectPath - the DBus object path to check
 * @param[i] value - the value to test against
 *
 * @return bool
 */
bool dbusObjectFilenameEquals(const std::string& objectName,
                              const std::string& value)
{
    sdbusplus::message::object_path objectPath(objectName);
    return (objectPath.filename() == value);
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
            const boost::system::error_code ec,
            const dbus::utility::MapperGetObject& objects) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "Error occurred GetObject";
            return;
        }

        std::optional<std::string> serviceName =
            checkAndGetSoleNonMapperGetObjectResponse(objects);
        if (!serviceName)
        {
            BMCWEB_LOG_ERROR << "Cannot get service from object name"
                             << diagnosticDataObjectPath;
            return;
        }

        sdbusplus::asio::getProperty<double>(
            *crow::connections::systemBus, *serviceName,
            diagnosticDataObjectPath, "xyz.openbmc_project.Sensor.Value",
            "Value",
            [asyncResp, diagnosticDataObjectPath](
                const boost::system::error_code ec2, double value) {
            if (ec2)
            {
                BMCWEB_LOG_DEBUG << "DBus response error on getProperty "
                                 << ec2;
                return;
            }

            // The names of the sensor objects should match those defined in
            // /etc/healthMon/bmc_health_config.json .
            if (dbusObjectFilenameEquals(diagnosticDataObjectPath,
                                         "CPU_Kernel"))
            {
                asyncResp->res
                    .jsonValue["ProcessorStatistics"]["KernelPercent"] = value;
            }
            else if (dbusObjectFilenameEquals(diagnosticDataObjectPath,
                                              "CPU_User"))
            {
                asyncResp->res.jsonValue["ProcessorStatistics"]["UserPercent"] =
                    value;
            }
            else if (dbusObjectFilenameEquals(diagnosticDataObjectPath,
                                              "Memory_AvailableBytes"))
            {
                asyncResp->res.jsonValue["MemoryStatistics"]["AvailableBytes"] =
                    static_cast<int64_t>(value);
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
                    const std::vector<std::string>& endpoints) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "DBus error getting association endpoints";
            return;
        }

        for (const std::string& endpoint : endpoints)
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
        [asyncResp, bmcObjectPath](
            const boost::system::error_code ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
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
        if (!pathAndService)
        {
            BMCWEB_LOG_ERROR << "Couldn't get sole Subtree response";
            return;
        }

        findBmcAssociationEndpoints(asyncResp, pathAndService->first,
                                    pathAndService->second);
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
                    const dbus::utility::MapperGetSubTreeResponse& subtree) {
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
        if (!pathAndService)
        {
            BMCWEB_LOG_ERROR << "Couldn't get sole Subtree response";
            return;
        }

        // Found BMC object path. Ask ObjectMapper about the reverse
        // Association edges.
        findBmcAssociation(asyncResp, pathAndService->first);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", int32_t(0),
        std::array<const char*, 1>{"xyz.openbmc_project.Inventory.Item.Bmc"});
}

/**
 * handleManagerDiagnosticData supports ManagerDiagnosticData.
 * It retrieves BMC health information from various DBus resources and returns
 * the information through the response.
 */
inline void handleManagerDiagnosticDataGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.jsonValue["@odata.type"] =
        "#ManagerDiagnosticData.v1_2_0.ManagerDiagnosticData";
    asyncResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/Managers/bmc/ManagerDiagnosticData";
    asyncResp->res.jsonValue["Id"] = "ManagerDiagnosticData";
    asyncResp->res.jsonValue["Name"] = "Manager Diagnostic Data";

    managerGetServiceRootUptime(asyncResp);

    // Start the first step of the 4-step process to populate
    // the BMC diagnostic data
    findBmcInventory(asyncResp);
}

inline void requestRoutesManagerDiagnosticData(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/ManagerDiagnosticData")
        .privileges(redfish::privileges::getManagerDiagnosticData)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleManagerDiagnosticDataGet, std::ref(app)));
}

} // namespace redfish
