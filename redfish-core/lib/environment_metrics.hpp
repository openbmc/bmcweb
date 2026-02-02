// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/control.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/sensor_utils.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace redfish
{

inline void afterGetPowerWatts(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& path,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& valuesDict)
{
    if (ec)
    {
        if (ec != boost::system::errc::io_error)
        {
            BMCWEB_LOG_ERROR("DBUS response error for PowerWatts {}", ec);
            messages::internalError(asyncResp->res);
        }
        return;
    }

    nlohmann::json item = nlohmann::json::object();

    /* Don't return an error for a failure to fill in properties from the
     * single sensor. Just skip adding it.
     */
    if (sensor_utils::objectExcerptToJson(
            path, chassisId,
            sensor_utils::ChassisSubNode::environmentMetricsNode, "power",
            valuesDict, item))
    {
        asyncResp->res.jsonValue["PowerWatts"] = std::move(item);
    }
}

inline void handleTotalPowerList(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const boost::system::error_code& ec,
    const std::shared_ptr<sensor_utils::SensorServicePathList>& sensorList)
{
    BMCWEB_LOG_DEBUG("handleTotalPowerList: {}", sensorList->size());

    if (ec)
    {
        if (ec != boost::system::errc::io_error)
        {
            BMCWEB_LOG_ERROR("D-Bus response error {}", ec);
            messages::internalError(asyncResp->res);
        }
        return;
    }

    // TotalPower cannot be supplied by multiple sensors
    if (sensorList->size() != 1)
    {
        if (sensorList->empty())
        {
            // None found, not an error
            return;
        }
        BMCWEB_LOG_ERROR("Too many total power sensors found {}. Expected 1.",
                         sensorList->size());
        messages::internalError(asyncResp->res);
        return;
    }

    const std::string& serviceName = (*sensorList)[0].first;
    const std::string& sensorPath = (*sensorList)[0].second;
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, serviceName, sensorPath,
        "xyz.openbmc_project.Sensor.Value",
        [asyncResp, chassisId,
         sensorPath](const boost::system::error_code& ec1,
                     const dbus::utility::DBusPropertiesMap& propertiesList) {
            afterGetPowerWatts(asyncResp, chassisId, sensorPath, ec1,
                               propertiesList);
        });
}

inline void getTotalPowerSensor(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const boost::system::error_code& ec,
    const sensor_utils::SensorServicePathList& sensorsServiceAndPath)
{
    BMCWEB_LOG_DEBUG("getTotalPowerSensor {}", sensorsServiceAndPath.size());

    if (ec)
    {
        if (ec != boost::system::errc::io_error)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec);
            messages::internalError(asyncResp->res);
        }
        // None found, not an error
        return;
    }

    if (sensorsServiceAndPath.empty())
    {
        // No power sensors implement Sensor.Purpose, not an error
        return;
    }

    // Create vector to hold list of sensors with totalPower purpose
    std::shared_ptr<sensor_utils::SensorServicePathList> sensorList =
        std::make_shared<sensor_utils::SensorServicePathList>();

    sensor_utils::getSensorsByPurpose(
        asyncResp, sensorsServiceAndPath,
        sensor_utils::SensorPurpose::totalPower, sensorList,
        std::bind_front(handleTotalPowerList, asyncResp, chassisId));
}

/**
 * @brief Find sensor providing totalPower and fill in response
 *
 * Multiple D-Bus calls are needed to find the sensor providing the totalPower
 * details:
 *
 * 1. Retrieve list of power sensors associated with specified chassis which
 * implement the Sensor.Purpose interface.
 *
 * 2. For each of those power sensors retrieve the actual purpose of the sensor
 * to find the sensor implementing totalPower purpose. Expect no more than
 * one sensor to implement this purpose.
 *
 * 3. If a totalPower sensor is found then retrieve its properties to fill in
 * PowerWatts in the response.
 *
 * @param asyncResp Response data
 * @param validChassisPath Path to chassis, caller confirms path is valid
 * @param chassisId Chassis id matching <validChassisPath>
 */
inline void getPowerWatts(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& validChassisPath,
                          const std::string& chassisId)
{
    BMCWEB_LOG_DEBUG("getPowerWatts: {}", validChassisPath);

    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Sensor.Purpose"};
    sensor_utils::getAllSensorObjects(
        validChassisPath, "/xyz/openbmc_project/sensors/power", interfaces, 1,
        std::bind_front(getTotalPowerSensor, asyncResp, chassisId));
}

inline void handleEnvironmentMetricsHead(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    auto respHandler = [asyncResp, chassisId](
                           const std::optional<std::string>& validChassisPath) {
        if (!validChassisPath)
        {
            messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
            return;
        }

        asyncResp->res.addHeader(
            boost::beast::http::field::link,
            "</redfish/v1/JsonSchemas/EnvironmentMetrics/EnvironmentMetrics.json>; rel=describedby");
    };

    redfish::chassis_utils::getValidChassisPath(asyncResp, chassisId,
                                                std::move(respHandler));
}

inline void doEnvironmentMetricsGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId,
    const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/EnvironmentMetrics/EnvironmentMetrics.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] =
        "#EnvironmentMetrics.v1_3_0.EnvironmentMetrics";
    asyncResp->res.jsonValue["Name"] = "Chassis Environment Metrics";
    asyncResp->res.jsonValue["Id"] = "EnvironmentMetrics";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/EnvironmentMetrics", chassisId);

    getPowerWatts(asyncResp, *validChassisPath, chassisId);
}

inline void handleEnvironmentMetricsGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        std::bind_front(doEnvironmentMetricsGet, asyncResp, chassisId));
}

inline void requestRoutesEnvironmentMetrics(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/EnvironmentMetrics/")
        .privileges(redfish::privileges::headEnvironmentMetrics)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleEnvironmentMetricsHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/EnvironmentMetrics/")
        .privileges(redfish::privileges::getEnvironmentMetrics)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleEnvironmentMetricsGet, std::ref(app)));
}

inline void afterGetPowerCapProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error for Power.Cap: {}", ec);
            messages::internalError(asyncResp->res);
        }
        return;
    }

    const uint32_t* defaultPowerCap = nullptr;
    const uint32_t* maxPowerCapValue = nullptr;
    const uint32_t* minPowerCapValue = nullptr;
    const uint32_t* powerCap = nullptr;
    const bool* powerCapEnable = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "DefaultPowerCap",
        defaultPowerCap, "MaxPowerCapValue", maxPowerCapValue,
        "MinPowerCapValue", minPowerCapValue, "PowerCap", powerCap,
        "PowerCapEnable", powerCapEnable);

    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    nlohmann::json& powerLimit = asyncResp->res.jsonValue["PowerLimitWatts"];

    constexpr uint32_t invalidPowerCapValue =
        std::numeric_limits<uint32_t>::max();

    if (maxPowerCapValue != nullptr)
    {
        powerLimit["AllowableMax"] = *maxPowerCapValue / 1000;
    }

    if (minPowerCapValue != nullptr)
    {
        powerLimit["AllowableMin"] = *minPowerCapValue / 1000;
    }

    if (powerCapEnable != nullptr)
    {
        if (*powerCapEnable)
        {
            powerLimit["ControlMode"] = control::ControlMode::Automatic;
        }
        else
        {
            powerLimit["ControlMode"] = control::ControlMode::Disabled;
        }
    }

    if (defaultPowerCap != nullptr && *defaultPowerCap != invalidPowerCapValue)
    {
        powerLimit["DefaultSetPoint"] = *defaultPowerCap / 1000;
    }

    if (powerCap != nullptr)
    {
        if (*powerCap == invalidPowerCapValue)
        {
            powerLimit["SetPoint"] = nullptr;
        }
        else
        {
            powerLimit["SetPoint"] = *powerCap / 1000;
        }
    }
}

inline void getPowerCapFromControl(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& service, const std::string& controlPath)
{
    BMCWEB_LOG_DEBUG("getPowerCapFromControl: {}", controlPath);

    dbus::utility::getAllProperties(
        service, controlPath, "xyz.openbmc_project.Control.Power.Cap",
        std::bind_front(afterGetPowerCapProperties, asyncResp));
}

inline void afterGetControlledByAssociation(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::MapperEndPoints& endpoints)
{
    if (ec)
    {
        if (ec.value() == EBADR)
        {
            BMCWEB_LOG_DEBUG("No controlled_by association found");
            return;
        }
        BMCWEB_LOG_ERROR("DBUS response error for association: {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    if (endpoints.empty())
    {
        BMCWEB_LOG_DEBUG("No control endpoints found");
        return;
    }

    const std::string& controlPath = endpoints[0];
    BMCWEB_LOG_DEBUG("Found control path: {}", controlPath);

    constexpr std::array<std::string_view, 1> powerCapInterface = {
        "xyz.openbmc_project.Control.Power.Cap"};

    dbus::utility::getDbusObject(
        controlPath, powerCapInterface,
        [asyncResp, controlPath](const boost::system::error_code& ec2,
                                 const dbus::utility::MapperGetObject& object) {
            if (ec2 || object.empty())
            {
                BMCWEB_LOG_DEBUG("No Power.Cap interface on control path");
                return;
            }
            getPowerCapFromControl(asyncResp, object.begin()->first,
                                   controlPath);
        });
}

inline void getPowerLimitWatts(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& processorPath)
{
    BMCWEB_LOG_DEBUG("getPowerLimitWatts: {}", processorPath);

    std::string associationPath = processorPath + "/controlled_by";

    dbus::utility::getAssociationEndPoints(
        associationPath,
        std::bind_front(afterGetControlledByAssociation, asyncResp));
}

inline void afterGetProcessorForEnvMetrics(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& processorId, const std::string& objectPath,
    const dbus::utility::/*unused*/ MapperServiceMap& /*unused*/)
{
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/EnvironmentMetrics/EnvironmentMetrics.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] =
        "#EnvironmentMetrics.v1_3_0.EnvironmentMetrics";
    asyncResp->res.jsonValue["Name"] = "Processor Environment Metrics";
    asyncResp->res.jsonValue["Id"] = "EnvironmentMetrics";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Systems/{}/Processors/{}/EnvironmentMetrics",
        BMCWEB_REDFISH_SYSTEM_URI_NAME, processorId);

    getPowerLimitWatts(asyncResp, objectPath);
}

inline void handleProcessorEnvMetricsSubtree(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& processorId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error: {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    constexpr std::array<std::string_view, 2> processorInterfaces = {
        "xyz.openbmc_project.Inventory.Item.Cpu",
        "xyz.openbmc_project.Inventory.Item.Accelerator"};

    for (const auto& [objectPath, serviceMap] : subtree)
    {
        sdbusplus::message::object_path path(objectPath);
        if (path.filename() != processorId)
        {
            continue;
        }

        for (const auto& [serviceName, interfaceList] : serviceMap)
        {
            if (std::ranges::find_first_of(
                    interfaceList, processorInterfaces) != interfaceList.end())
            {
                afterGetProcessorForEnvMetrics(asyncResp, processorId,
                                               objectPath, serviceMap);
                return;
            }
        }
    }

    messages::resourceNotFound(asyncResp->res, "Processor", processorId);
}

inline void getProcessorForEnvMetrics(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& processorId)
{
    BMCWEB_LOG_DEBUG("getProcessorForEnvMetrics: {}", processorId);

    constexpr std::array<std::string_view, 2> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Cpu",
        "xyz.openbmc_project.Inventory.Item.Accelerator"};

    dbus::utility::getSubTree("/xyz/openbmc_project/inventory", 0, interfaces,
                              std::bind_front(handleProcessorEnvMetricsSubtree,
                                              asyncResp, processorId));
}

inline void handleProcessorEnvironmentMetricsHead(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& processorId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems. TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/EnvironmentMetrics/EnvironmentMetrics.json>; rel=describedby");

    getProcessorForEnvMetrics(asyncResp, processorId);
}

inline void handleProcessorEnvironmentMetricsGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& processorId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems. TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    getProcessorForEnvMetrics(asyncResp, processorId);
}

inline void requestRoutesProcessorEnvironmentMetrics(App& app)
{
    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/<str>/Processors/<str>/EnvironmentMetrics/")
        .privileges(redfish::privileges::headEnvironmentMetrics)
        .methods(boost::beast::http::verb::head)(std::bind_front(
            handleProcessorEnvironmentMetricsHead, std::ref(app)));

    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/<str>/Processors/<str>/EnvironmentMetrics/")
        .privileges(redfish::privileges::getEnvironmentMetrics)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleProcessorEnvironmentMetricsGet, std::ref(app)));
}

} // namespace redfish
