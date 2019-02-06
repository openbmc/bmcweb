/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#pragma once

#include <boost/container/flat_map.hpp>
#include <node.hpp>
#include <utils/json_utils.hpp>
#include <variant>
#include <xyz/openbmc_project/Control/Boot/Mode/server.hpp>
#include <xyz/openbmc_project/Control/Boot/Source/server.hpp>

namespace redfish
{

/**
 * @brief Retrieves computer system properties over dbus
 *
 * @param[in] aResp Shared pointer for completing asynchronous calls
 * @param[in] name  Computer system name from request
 *
 * @return None.
 */
void getComputerSystem(std::shared_ptr<AsyncResp> aResp,
                       const std::string &name)
{
    BMCWEB_LOG_DEBUG << "Get available system components.";
    crow::connections::systemBus->async_method_call(
        [name, aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>
                &subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }
            bool foundName = false;
            // Iterate over all retrieved ObjectPaths.
            for (const std::pair<std::string,
                                 std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>
                     &object : subtree)
            {
                const std::string &path = object.first;
                BMCWEB_LOG_DEBUG << "Got path: " << path;
                const std::vector<
                    std::pair<std::string, std::vector<std::string>>>
                    &connectionNames = object.second;
                if (connectionNames.size() < 1)
                {
                    continue;
                }
                // Check if computer system exist
                if (boost::ends_with(path, name))
                {
                    foundName = true;
                    BMCWEB_LOG_DEBUG << "Found name: " << name;
                    const std::string connectionName = connectionNames[0].first;
                    crow::connections::systemBus->async_method_call(
                        [aResp, name(std::string(name))](
                            const boost::system::error_code ec,
                            const std::vector<std::pair<
                                std::string, VariantType>> &propertiesList) {
                            if (ec)
                            {
                                BMCWEB_LOG_ERROR << "DBUS response error: "
                                                 << ec;
                                messages::internalError(aResp->res);
                                return;
                            }
                            BMCWEB_LOG_DEBUG << "Got " << propertiesList.size()
                                             << "properties for system";
                            for (const std::pair<std::string, VariantType>
                                     &property : propertiesList)
                            {
                                const std::string *value =
                                    std::get_if<std::string>(&property.second);
                                if (value != nullptr)
                                {
                                    aResp->res.jsonValue[property.first] =
                                        *value;
                                }
                            }
                            aResp->res.jsonValue["Name"] = name;
                            aResp->res.jsonValue["Id"] =
                                aResp->res.jsonValue["SerialNumber"];
                        },
                        connectionName, path, "org.freedesktop.DBus.Properties",
                        "GetAll",
                        "xyz.openbmc_project.Inventory.Decorator.Asset");
                }
                else
                {
                    // This is not system, so check if it's cpu, dimm, UUID or
                    // BiosVer
                    for (const auto &connection : connectionNames)
                    {
                        for (const auto &interfaceName : connection.second)
                        {
                            if (interfaceName ==
                                "xyz.openbmc_project.Inventory.Item.Dimm")
                            {
                                BMCWEB_LOG_DEBUG
                                    << "Found Dimm, now get its properties.";
                                crow::connections::systemBus->async_method_call(
                                    [aResp](
                                        const boost::system::error_code ec,
                                        const std::vector<
                                            std::pair<std::string, VariantType>>
                                            &properties) {
                                        if (ec)
                                        {
                                            BMCWEB_LOG_ERROR
                                                << "DBUS response error " << ec;
                                            messages::internalError(aResp->res);
                                            return;
                                        }
                                        BMCWEB_LOG_DEBUG << "Got "
                                                         << properties.size()
                                                         << "Dimm properties.";
                                        for (const std::pair<std::string,
                                                             VariantType>
                                                 &property : properties)
                                        {
                                            if (property.first ==
                                                "MemorySizeInKb")
                                            {
                                                const uint64_t *value =
                                                    sdbusplus::message::
                                                        variant_ns::get_if<
                                                            uint64_t>(
                                                            &property.second);
                                                if (value != nullptr)
                                                {
                                                    aResp->res.jsonValue
                                                        ["TotalSystemMemoryGi"
                                                         "B"] +=
                                                        *value / (1024 * 1024);
                                                    aResp->res.jsonValue
                                                        ["MemorySummary"]
                                                        ["Status"]["State"] =
                                                        "Enabled";
                                                }
                                            }
                                        }
                                    },
                                    connection.first, path,
                                    "org.freedesktop.DBus.Properties", "GetAll",
                                    "xyz.openbmc_project.Inventory.Item.Dimm");
                            }
                            else if (interfaceName ==
                                     "xyz.openbmc_project.Inventory.Item.Cpu")
                            {
                                BMCWEB_LOG_DEBUG
                                    << "Found Cpu, now get its properties.";
                                crow::connections::systemBus->async_method_call(
                                    [aResp](
                                        const boost::system::error_code ec,
                                        const std::vector<
                                            std::pair<std::string, VariantType>>
                                            &properties) {
                                        if (ec)
                                        {
                                            BMCWEB_LOG_ERROR
                                                << "DBUS response error " << ec;
                                            messages::internalError(aResp->res);
                                            return;
                                        }
                                        BMCWEB_LOG_DEBUG << "Got "
                                                         << properties.size()
                                                         << "Cpu properties.";
                                        for (const auto &property : properties)
                                        {
                                            if (property.first ==
                                                "ProcessorFamily")
                                            {
                                                const std::string *value =
                                                    sdbusplus::message::
                                                        variant_ns::get_if<
                                                            std::string>(
                                                            &property.second);
                                                if (value != nullptr)
                                                {
                                                    nlohmann::json
                                                        &procSummary =
                                                            aResp->res.jsonValue
                                                                ["ProcessorSumm"
                                                                 "ary"];
                                                    nlohmann::json &procCount =
                                                        procSummary["Count"];

                                                    procCount =
                                                        procCount.get<int>() +
                                                        1;
                                                    procSummary["Status"]
                                                               ["State"] =
                                                                   "Enabled";
                                                    procSummary["Model"] =
                                                        *value;
                                                }
                                            }
                                        }
                                    },
                                    connection.first, path,
                                    "org.freedesktop.DBus.Properties", "GetAll",
                                    "xyz.openbmc_project.Inventory.Item.Cpu");
                            }
                            else if (interfaceName ==
                                     "xyz.openbmc_project.Common.UUID")
                            {
                                BMCWEB_LOG_DEBUG
                                    << "Found UUID, now get its properties.";
                                crow::connections::systemBus->async_method_call(
                                    [aResp](
                                        const boost::system::error_code ec,
                                        const std::vector<
                                            std::pair<std::string, VariantType>>
                                            &properties) {
                                        if (ec)
                                        {
                                            BMCWEB_LOG_DEBUG
                                                << "DBUS response error " << ec;
                                            messages::internalError(aResp->res);
                                            return;
                                        }
                                        BMCWEB_LOG_DEBUG << "Got "
                                                         << properties.size()
                                                         << "UUID properties.";
                                        for (const std::pair<std::string,
                                                             VariantType>
                                                 &property : properties)
                                        {
                                            if (property.first == "BIOSVer")
                                            {
                                                const std::string *value =
                                                    sdbusplus::message::
                                                        variant_ns::get_if<
                                                            std::string>(
                                                            &property.second);
                                                if (value != nullptr)
                                                {
                                                    aResp->res.jsonValue
                                                        ["BiosVersion"] =
                                                        *value;
                                                }
                                            }
                                            if (property.first == "UUID")
                                            {
                                                const std::string *value =
                                                    sdbusplus::message::
                                                        variant_ns::get_if<
                                                            std::string>(
                                                            &property.second);

                                                if (value != nullptr)
                                                {
                                                    std::string valueStr =
                                                        *value;
                                                    if (valueStr.size() == 32)
                                                    {
                                                        valueStr.insert(8, 1,
                                                                        '-');
                                                        valueStr.insert(13, 1,
                                                                        '-');
                                                        valueStr.insert(18, 1,
                                                                        '-');
                                                        valueStr.insert(23, 1,
                                                                        '-');
                                                    }
                                                    BMCWEB_LOG_DEBUG
                                                        << "UUID = "
                                                        << valueStr;
                                                    aResp->res
                                                        .jsonValue["UUID"] =
                                                        valueStr;
                                                }
                                            }
                                        }
                                    },
                                    connection.first, path,
                                    "org.freedesktop.DBus.Properties", "GetAll",
                                    "xyz.openbmc_project.Common.UUID");
                            }
                        }
                    }
                }
            }
            if (foundName == false)
            {
                messages::internalError(aResp->res);
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", int32_t(0),
        std::array<const char *, 5>{
            "xyz.openbmc_project.Inventory.Decorator.Asset",
            "xyz.openbmc_project.Inventory.Item.Cpu",
            "xyz.openbmc_project.Inventory.Item.Dimm",
            "xyz.openbmc_project.Inventory.Item.System",
            "xyz.openbmc_project.Common.UUID",
        });
}

/**
 * @brief Retrieves identify led group properties over dbus
 *
 * @param[in] aResp     Shared pointer for completing asynchronous calls.
 * @param[in] callback  Callback for process retrieved data.
 *
 * @return None.
 */
template <typename CallbackFunc>
void getLedGroupIdentify(std::shared_ptr<AsyncResp> aResp,
                         CallbackFunc &&callback)
{
    BMCWEB_LOG_DEBUG << "Get led groups";
    crow::connections::systemBus->async_method_call(
        [aResp{std::move(aResp)},
         callback{std::move(callback)}](const boost::system::error_code &ec,
                                        const ManagedObjectsType &resp) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                messages::internalError(aResp->res);
                return;
            }
            BMCWEB_LOG_DEBUG << "Got " << resp.size() << "led group objects.";
            for (const auto &objPath : resp)
            {
                const std::string &path = objPath.first;
                if (path.rfind("enclosure_identify") != std::string::npos)
                {
                    for (const auto &interface : objPath.second)
                    {
                        if (interface.first == "xyz.openbmc_project.Led.Group")
                        {
                            for (const auto &property : interface.second)
                            {
                                if (property.first == "Asserted")
                                {
                                    const bool *asserted =
                                        std::get_if<bool>(&property.second);
                                    if (nullptr != asserted)
                                    {
                                        callback(*asserted, aResp);
                                    }
                                    else
                                    {
                                        callback(false, aResp);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        },
        "xyz.openbmc_project.LED.GroupManager",
        "/xyz/openbmc_project/led/groups", "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects");
}

template <typename CallbackFunc>
void getLedIdentify(std::shared_ptr<AsyncResp> aResp, CallbackFunc &&callback)
{
    BMCWEB_LOG_DEBUG << "Get identify led properties";
    crow::connections::systemBus->async_method_call(
        [aResp,
         callback{std::move(callback)}](const boost::system::error_code ec,
                                        const PropertiesType &properties) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                messages::internalError(aResp->res);
                return;
            }
            BMCWEB_LOG_DEBUG << "Got " << properties.size()
                             << "led properties.";
            std::string output;
            for (const auto &property : properties)
            {
                if (property.first == "State")
                {
                    const std::string *s =
                        std::get_if<std::string>(&property.second);
                    if (nullptr != s)
                    {
                        BMCWEB_LOG_DEBUG << "Identify Led State: " << *s;
                        const auto pos = s->rfind('.');
                        if (pos != std::string::npos)
                        {
                            auto led = s->substr(pos + 1);
                            for (const std::pair<const char *, const char *>
                                     &p :
                                 std::array<
                                     std::pair<const char *, const char *>, 3>{
                                     {{"On", "Lit"},
                                      {"Blink", "Blinking"},
                                      {"Off", "Off"}}})
                            {
                                if (led == p.first)
                                {
                                    output = p.second;
                                }
                            }
                        }
                    }
                }
            }
            callback(output, aResp);
        },
        "xyz.openbmc_project.LED.Controller.identify",
        "/xyz/openbmc_project/led/physical/identify",
        "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.Led.Physical");
}

/**
 * @brief Retrieves host state properties over dbus
 *
 * @param[in] aResp     Shared pointer for completing asynchronous calls.
 *
 * @return None.
 */
void getHostState(std::shared_ptr<AsyncResp> aResp)
{
    BMCWEB_LOG_DEBUG << "Get host information.";
    crow::connections::systemBus->async_method_call(
        [aResp{std::move(aResp)}](const boost::system::error_code ec,
                                  const std::variant<std::string> &hostState) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                messages::internalError(aResp->res);
                return;
            }

            const std::string *s = std::get_if<std::string>(&hostState);
            BMCWEB_LOG_DEBUG << "Host state: " << *s;
            if (s != nullptr)
            {
                // Verify Host State
                if (*s == "xyz.openbmc_project.State.Host.HostState.Running")
                {
                    aResp->res.jsonValue["PowerState"] = "On";
                    aResp->res.jsonValue["Status"]["State"] = "Enabled";
                }
                else
                {
                    aResp->res.jsonValue["PowerState"] = "Off";
                    aResp->res.jsonValue["Status"]["State"] = "Disabled";
                }
            }
        },
        "xyz.openbmc_project.State.Host", "/xyz/openbmc_project/state/host0",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.State.Host", "CurrentHostState");
}

namespace boot
{
using namespace sdbusplus::xyz::openbmc_project::Control::Boot::server;
// Maps dbus property values to Redfish and vice-versa
boost::container::flat_map<Source::Sources, std::string> dbusToRfSource = {
    {Source::Sources::Default, "None"},
    {Source::Sources::Disk, "Hdd"},
    {Source::Sources::ExternalMedia, "Cd"},
    {Source::Sources::Network, "Pxe"}};

boost::container::flat_map<std::string, Source::Sources> rfToDbusSource = {
    {"None", Source::Sources::Default},
    {"Hdd", Source::Sources::Disk},
    {"Cd", Source::Sources::ExternalMedia},
    {"Pxe", Source::Sources::Network}};

boost::container::flat_map<Mode::Modes, std::string> dbusToRfMode = {
    {Mode::Modes::Safe, "Diags"},
    {Mode::Modes::Setup, "BiosSetup"},
    {Mode::Modes::Regular, "None"}};

boost::container::flat_map<std::string, Mode::Modes> rfToDbusMode = {
    {"Diags", Mode::Modes::Safe},
    {"BiosSetup", Mode::Modes::Setup},
    {"None", Mode::Modes::Regular}};
} // namespace boot

/**
 * @brief Retrieves boot mode over DBUS and fills out the response
 *
 * @param[in] aResp         Shared pointer for completing asynchronous calls.
 * @param[in] bootDbusObj   The dbus object to query for boot properties.
 *
 * @return None.
 */
static void getBootMode(std::shared_ptr<AsyncResp> aResp,
                        const std::string &bootDbusObj)
{
    using namespace boot;
    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                const sdbusplus::message::variant<std::string> &bootMode) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                messages::internalError(aResp->res);
                return;
            }

            const std::string *s =
                sdbusplus::message::variant_ns::get_if<std::string>(&bootMode);

            if (s)
            {
                BMCWEB_LOG_DEBUG << "Boot mode: " << *s;
            }

            // TODO: Do we need to support override mode?
            aResp->res.jsonValue["Boot"]["BootSourceOverrideMode"] = "Legacy";
            aResp->res.jsonValue["Boot"]["BootSourceOverrideTarget@Redfish."
                                         "AllowableValues"] = {
                "None", "Pxe", "Hdd", "Cd", "Diags", "BiosSetup"};
            Mode::Modes mode = Mode::convertModesFromString(*s);
            if (Mode::Modes::Regular != mode)
            {
                aResp->res.jsonValue["Boot"]["BootSourceOverrideTarget"] =
                    boot::dbusToRfMode.at(mode);
            }

            // If the BootSourceOverrideTarget is still "None" at the end, reset
            // the BootSourceOverrideEnabled to indicate that overrides are
            // disabled
            if (aResp->res.jsonValue["Boot"]["BootSourceOverrideTarget"] ==
                "None")
            {
                aResp->res.jsonValue["Boot"]["BootSourceOverrideEnabled"] =
                    "Disabled";
            }
        },
        "xyz.openbmc_project.Settings", bootDbusObj,
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Control.Boot.Mode", "BootMode");
}

/**
 * @brief Retrieves boot source over DBUS
 *
 * @param[in] aResp         Shared pointer for completing asynchronous calls.
 * @param[in] oneTimeEnable Boolean to indicate boot properties are one-time.
 *
 * @return None.
 */
static void getBootSource(std::shared_ptr<AsyncResp> aResp, bool oneTimeEnabled,
                          std::optional<nlohmann::json> /*unused*/)
{
    using namespace boot;

    std::string bootDbusObj =
        (oneTimeEnabled) ? "/xyz/openbmc_project/control/host0/boot/one_time"
                         : "/xyz/openbmc_project/control/host0/boot";

    BMCWEB_LOG_DEBUG << "Is one time: " << oneTimeEnabled;
    aResp->res.jsonValue["Boot"]["BootSourceOverrideEnabled"] =
        (oneTimeEnabled) ? "Once" : "Continuous";

    crow::connections::systemBus->async_method_call(
        [aResp{std::move(aResp)}, bootDbusObj](
            const boost::system::error_code ec,
            const sdbusplus::message::variant<std::string> &bootSource) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                messages::internalError(aResp->res);
                return;
            }

            const std::string *s =
                sdbusplus::message::variant_ns::get_if<std::string>(
                    &bootSource);
            if (s)
            {
                BMCWEB_LOG_DEBUG << "Boot source: " << *s;
                aResp->res.jsonValue["Boot"]["BootSourceOverrideTarget"] =
                    boot::dbusToRfSource.at(
                        Source::convertSourcesFromString(*s));
            }
            getBootMode(aResp, std::move(bootDbusObj));
        },
        "xyz.openbmc_project.Settings", bootDbusObj,
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Control.Boot.Source", "BootSource");
}

/**
 * @brief Retrieves "One time" enabled setting over DBUS and forwards the result
 * to a user supplied (async) callback function for further processing.
 *
 * @param[in] aResp     Shared pointer for completing asynchronous calls.
 * @param[in] callback  Callback for further processing.
 * @param[in] bootProps JSON data containing boot props. Unused when doing a GET
 *
 * @return None.
 */
template <typename CallbackFunc>
static void handleBootProperties(std::shared_ptr<AsyncResp> aResp,
                                 CallbackFunc &&callback,
                                 std::optional<nlohmann::json> bootProps)
{
    BMCWEB_LOG_DEBUG << "Get boot information.";

    crow::connections::systemBus->async_method_call(
        [aResp{std::move(aResp)}, callback{std::move(callback)},
         bootProps{std::move(bootProps)}](
            const boost::system::error_code ec,
            const sdbusplus::message::variant<bool> &oneTime) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                messages::internalError(aResp->res);
                return;
            }

            const bool *b =
                sdbusplus::message::variant_ns::get_if<bool>(&oneTime);

            callback(aResp, *b, bootProps);
        },
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/boot/one_time",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Object.Enable", "Enabled");
}

/**
 * @brief Sets boot properties into DBUS object(s).
 *
 * @param[in] aResp           Shared pointer for completing asynchronous calls.
 * @param[in] oneTimeEnabled  Is "one-time" setting already enabled.
 * @param[in] json            JSON data containing boot props.
 *
 * @return None.
 */
static void setBootProperties(std::shared_ptr<AsyncResp> aResp,
                              bool oneTimeEnabled,
                              std::optional<nlohmann::json> json)
{
    using namespace boot;
    const auto itr = (*json).find("BootSourceOverrideTarget");
    const auto itr1 = (*json).find("BootSourceOverrideEnabled");

    const std::string *bootSourceTarget =
        (itr != (*json).end()) ? (*itr).get_ptr<const std::string *>()
                               : nullptr;
    const std::string *bootSourceEnable =
        (itr1 != (*json).end()) ? (*itr1).get_ptr<const std::string *>()
                                : nullptr;

    bool oneTimeSetting =
        (bootSourceEnable) ? (*bootSourceEnable == "Once") : oneTimeEnabled;

    std::string bootObj =
        (oneTimeSetting) ? "/xyz/openbmc_project/control/host0/boot/one_time"
                         : "/xyz/openbmc_project/control/host0/boot";
    struct DbusOp
    {
        std::string obj;
        std::string iface;
        std::string prop;
        std::string val;

        DbusOp(const std::string &o, const std::string &i, const std::string &p,
               const std::string &v) :
            obj(o),
            iface(i), prop(p), val(v)
        {
        }
    };

    using DbusOps = std::vector<DbusOp>;
    DbusOps busOps;

    BMCWEB_LOG_DEBUG << "Got source: " << bootSourceTarget
                     << ", enable: " << bootSourceEnable;

    // Figure out what properties to set
    if (!bootSourceTarget && !bootSourceEnable)
    {
        // No-op
        return;
    }
    else if (!bootSourceTarget && bootSourceEnable)
    {
        BMCWEB_LOG_DEBUG << "Boot source enabled: " << *bootSourceEnable;
        // Request to only turn OFF/ON enabled, if turning enabled OFF, need
        // to reset the source and mode too. If turning it ON, we only need
        // to set the enabled property
        if (*bootSourceEnable == "Disabled")
        {
            busOps.reserve(2);
            busOps.emplace_back(DbusOp{
                bootObj, "xyz.openbmc_project.Control.Boot.Source",
                "BootSource", convertForMessage(Source::Sources::Default)});
            busOps.emplace_back(
                DbusOp{bootObj, "xyz.openbmc_project.Control.Boot.Mode",
                       "BootMode", convertForMessage(Mode::Modes::Regular)});
        }
    }
    else if (bootSourceTarget)
    {
        // Source target specified
        BMCWEB_LOG_DEBUG << "Boot source: " << *bootSourceTarget;
        busOps.reserve(2);
        // Figure out which DBUS interface and property to use
        const auto bootSourceItr = rfToDbusSource.find(*bootSourceTarget);
        const auto bootModeItr = rfToDbusMode.find(*bootSourceTarget);

        if ((rfToDbusSource.end() == bootSourceItr) &&
            (rfToDbusMode.end() == bootModeItr))
        {
            BMCWEB_LOG_DEBUG << "Invalid property value for "
                                "BootSourceOverrideTarget: "
                             << *bootSourceTarget;
            messages::propertyValueNotInList(aResp->res, *bootSourceTarget,
                                             "BootSourceTargetOverride");
            return;
        }

        if (bootSourceItr != rfToDbusSource.end())
        {
            busOps.emplace_back(
                DbusOp{bootObj, "xyz.openbmc_project.Control.Boot.Source",
                       "BootSource", convertForMessage(bootSourceItr->second)});
            // If setting to anything other than default, also reset boot
            // mode property
            if (Source::Sources::Default != bootSourceItr->second)
            {
                busOps.emplace_back(DbusOp{
                    bootObj, "xyz.openbmc_project.Control.Boot.Mode",
                    "BootMode", convertForMessage(Mode::Modes::Regular)});
            }
        }
        else
        {
            busOps.emplace_back(
                DbusOp{bootObj, "xyz.openbmc_project.Control.Boot.Mode",
                       "BootMode", convertForMessage(bootModeItr->second)});
            // If setting to anything other than default, also reset boot
            // source property
            if (Mode::Modes::Regular != bootModeItr->second)
            {
                busOps.emplace_back(DbusOp{
                    bootObj, "xyz.openbmc_project.Control.Boot.Source",
                    "BootSource", convertForMessage(Source::Sources::Default)});
            }
        }
    }
    // Do bus operations
    for (const auto &busOp : busOps)
    {
        BMCWEB_LOG_DEBUG << "DBUS args:: " << busOp.obj << ", " << busOp.iface
                         << ", " << busOp.prop << ", " << busOp.val;

        crow::connections::systemBus->async_method_call(
            [aResp](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                    messages::internalError(aResp->res);
                    return;
                }
                BMCWEB_LOG_DEBUG << "Boot source/mode update done.";
            },
            "xyz.openbmc_project.Settings", busOp.obj,
            "org.freedesktop.DBus.Properties", "Set", busOp.iface, busOp.prop,
            sdbusplus::message::variant<std::string>{busOp.val});
    }
    if (oneTimeSetting != oneTimeEnabled)
    {
        // Write one time enabled/disabled out to the DBUS object only if
        // changed
        crow::connections::systemBus->async_method_call(
            [aResp{std::move(aResp)}](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                    messages::internalError(aResp->res);
                    return;
                }
                BMCWEB_LOG_DEBUG << "Boot enable update done.";
            },
            "xyz.openbmc_project.Settings",
            "/xyz/openbmc_project/control/host0/boot/one_time",
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Object.Enable", "Enabled",
            sdbusplus::message::variant<bool>(oneTimeSetting));
    }
}

/**
 * SystemsCollection derived class for delivering ComputerSystems Collection
 * Schema
 */
class SystemsCollection : public Node
{
  public:
    SystemsCollection(CrowApp &app) : Node(app, "/redfish/v1/Systems/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        BMCWEB_LOG_DEBUG << "Get list of available boards.";
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        res.jsonValue["@odata.type"] =
            "#ComputerSystemCollection.ComputerSystemCollection";
        res.jsonValue["@odata.id"] = "/redfish/v1/Systems";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/"
            "$metadata#ComputerSystemCollection.ComputerSystemCollection";
        res.jsonValue["Name"] = "Computer System Collection";
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const std::vector<std::string> &resp) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                BMCWEB_LOG_DEBUG << "Got " << resp.size() << " boards.";

                // ... prepare json array with appropriate @odata.id links
                nlohmann::json &boardArray =
                    asyncResp->res.jsonValue["Members"];
                boardArray = nlohmann::json::array();

                // Iterate over all retrieved ObjectPaths.
                for (const std::string &objpath : resp)
                {
                    std::size_t lastPos = objpath.rfind("/");
                    if (lastPos != std::string::npos)
                    {
                        boardArray.push_back(
                            {{"@odata.id", "/redfish/v1/Systems/" +
                                               objpath.substr(lastPos + 1)}});
                    }
                }

                asyncResp->res.jsonValue["Members@odata.count"] =
                    boardArray.size();
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
            "/xyz/openbmc_project/inventory", int32_t(0),
            std::array<const char *, 1>{
                "xyz.openbmc_project.Inventory.Item.Board"});
    }
};

/**
 * SystemActionsReset class supports handle POST method for Reset action.
 * The class retrieves and sends data directly to D-Bus.
 */
class SystemActionsReset : public Node
{
  public:
    SystemActionsReset(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/<str>/Actions/ComputerSystem.Reset/",
             std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    /**
     * Function handles POST method request.
     * Analyzes POST body message before sends Reset request data to D-Bus.
     */
    void doPost(crow::Response &res, const crow::Request &req,
                const std::vector<std::string> &params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);

        std::string resetType;
        if (!json_util::readJson(req, res, "ResetType", resetType))
        {
            return;
        }

        if (resetType == "ForceOff")
        {
            // Force off acts on the chassis
            crow::connections::systemBus->async_method_call(
                [asyncResp](const boost::system::error_code ec) {
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR << "D-Bus responses error: " << ec;
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    // TODO Consider support polling mechanism to verify
                    // status of host and chassis after execute the
                    // requested action.
                    messages::success(asyncResp->res);
                },
                "xyz.openbmc_project.State.Chassis",
                "/xyz/openbmc_project/state/chassis0",
                "org.freedesktop.DBus.Properties", "Set",
                "xyz.openbmc_project.State.Chassis", "RequestedPowerTransition",
                std::variant<std::string>{
                    "xyz.openbmc_project.State.Chassis.Transition.Off"});
            return;
        }
        // all other actions operate on the host
        std::string command;
        // Execute Reset Action regarding to each reset type.
        if (resetType == "On")
        {
            command = "xyz.openbmc_project.State.Host.Transition.On";
        }
        else if (resetType == "GracefulShutdown")
        {
            command = "xyz.openbmc_project.State.Host.Transition.Off";
        }
        else if (resetType == "GracefulRestart")
        {
            command = "xyz.openbmc_project.State.Host.Transition.Reboot";
        }
        else
        {
            messages::actionParameterUnknown(res, "Reset", resetType);
            return;
        }

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "D-Bus responses error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                // TODO Consider support polling mechanism to verify
                // status of host and chassis after execute the
                // requested action.
                messages::success(asyncResp->res);
            },
            "xyz.openbmc_project.State.Host",
            "/xyz/openbmc_project/state/host0",
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.State.Host", "RequestedHostTransition",
            std::variant<std::string>{command});
    }
};

/**
 * Systems derived class for delivering Computer Systems Schema.
 */
class Systems : public Node
{
  public:
    /*
     * Default Constructor
     */
    Systems(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/<str>/", std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    /**
     * Functions triggers appropriate requests on DBus
     */
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        // Check if there is required param, truly entering this shall be
        // impossible
        if (params.size() != 1)
        {
            messages::internalError(res);
            res.end();
            return;
        }

        const std::string &name = params[0];

        res.jsonValue["@odata.type"] = "#ComputerSystem.v1_6_0.ComputerSystem";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#ComputerSystem.ComputerSystem";
        res.jsonValue["SystemType"] = "Physical";
        res.jsonValue["Description"] = "Computer System";
        res.jsonValue["ProcessorSummary"]["Count"] = 0;
        res.jsonValue["ProcessorSummary"]["Status"]["State"] = "Disabled";
        res.jsonValue["MemorySummary"]["TotalSystemMemoryGiB"] = int(0);
        res.jsonValue["MemorySummary"]["Status"]["State"] = "Disabled";
        res.jsonValue["@odata.id"] = "/redfish/v1/Systems/" + name;

        res.jsonValue["Processors"] = {
            {"@odata.id", "/redfish/v1/Systems/" + name + "/Processors"}};
        res.jsonValue["Memory"] = {
            {"@odata.id", "/redfish/v1/Systems/" + name + "/Memory"}};
        // TODO Need to support ForceRestart.
        res.jsonValue["Actions"]["#ComputerSystem.Reset"] = {
            {"target",
             "/redfish/v1/Systems/" + name + "/Actions/ComputerSystem.Reset"},
            {"ResetType@Redfish.AllowableValues",
             {"On", "ForceOff", "GracefulRestart", "GracefulShutdown"}}};

        res.jsonValue["LogServices"] = {
            {"@odata.id", "/redfish/v1/Systems/" + name + "/LogServices"}};

        auto asyncResp = std::make_shared<AsyncResp>(res);

        getLedGroupIdentify(
            asyncResp,
            [&](const bool &asserted, const std::shared_ptr<AsyncResp> &aResp) {
                if (asserted)
                {
                    // If led group is asserted, then another call is needed to
                    // get led status
                    getLedIdentify(
                        aResp, [](const std::string &ledStatus,
                                  const std::shared_ptr<AsyncResp> &aResp) {
                            if (!ledStatus.empty())
                            {
                                aResp->res.jsonValue["IndicatorLED"] =
                                    ledStatus;
                            }
                        });
                }
                else
                {
                    aResp->res.jsonValue["IndicatorLED"] = "Off";
                }
            });
        getComputerSystem(asyncResp, name);
        getHostState(asyncResp);
        handleBootProperties(asyncResp, getBootSource, std::nullopt);
    }

    void doPatch(crow::Response &res, const crow::Request &req,
                 const std::vector<std::string> &params) override
    {
        // Check if there is required param, truly entering this shall be
        // impossible
        auto asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string &name = params[0];

        messages::success(asyncResp->res);

        std::string indicatorLedTemp;
        std::optional<std::string> indicatorLed = indicatorLedTemp;
        std::optional<nlohmann::json> bootProps;
        if (!json_util::readJson(req, res, "IndicatorLED", indicatorLed, "Boot",
                                 bootProps))
        {
            return;
        }

        if (bootProps)
        {
            handleBootProperties(asyncResp, setBootProperties, bootProps);
        }

        if (indicatorLed)
        {
            std::string dbusLedState;
            if (*indicatorLed == "On")
            {
                dbusLedState = "xyz.openbmc_project.Led.Physical.Action.Lit";
            }
            else if (*indicatorLed == "Blink")
            {
                dbusLedState =
                    "xyz.openbmc_project.Led.Physical.Action.Blinking";
            }
            else if (*indicatorLed == "Off")
            {
                dbusLedState = "xyz.openbmc_project.Led.Physical.Action.Off";
            }
            else
            {
                messages::propertyValueNotInList(res, *indicatorLed,
                                                 "IndicatorLED");
                return;
            }

            getHostState(asyncResp);
            getComputerSystem(asyncResp, name);

            // Update led group
            BMCWEB_LOG_DEBUG << "Update led group.";
            crow::connections::systemBus->async_method_call(
                [asyncResp{std::move(asyncResp)}](
                    const boost::system::error_code ec) {
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    BMCWEB_LOG_DEBUG << "Led group update done.";
                },
                "xyz.openbmc_project.LED.GroupManager",
                "/xyz/openbmc_project/led/groups/enclosure_identify",
                "org.freedesktop.DBus.Properties", "Set",
                "xyz.openbmc_project.Led.Group", "Asserted",
                std::variant<bool>(
                    (dbusLedState ==
                             "xyz.openbmc_project.Led.Physical.Action.Off"
                         ? false
                         : true)));
            // Update identify led status
            BMCWEB_LOG_DEBUG << "Update led SoftwareInventoryCollection.";
            crow::connections::systemBus->async_method_call(
                [asyncResp{std::move(asyncResp)},
                 indicatorLed{std::move(*indicatorLed)}](
                    const boost::system::error_code ec) {
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    BMCWEB_LOG_DEBUG << "Led state update done.";
                },
                "xyz.openbmc_project.LED.Controller.identify",
                "/xyz/openbmc_project/led/physical/identify",
                "org.freedesktop.DBus.Properties", "Set",
                "xyz.openbmc_project.Led.Physical", "State",
                std::variant<std::string>(dbusLedState));
        }
    }
};
} // namespace redfish
