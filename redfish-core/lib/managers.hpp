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

#include "health_class_decl.hpp"
#include "redfish_util.hpp"

#include <app_class_decl.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/date_time.hpp>
#include <dbus_utility.hpp>
#include <registries/privilege_registry.hpp>
#include <utils/fw_utils.hpp>
#include <utils/systemd_utils.hpp>

#include <cstdint>
#include <memory>
#include <sstream>
#include <variant>


using DbusVariantType =
    std::variant<std::vector<std::tuple<std::string, std::string, std::string>>,
                 std::vector<std::string>, std::vector<double>, std::string,
                 int64_t, uint64_t, double, int32_t, uint32_t, int16_t,
                 uint16_t, uint8_t, bool>;
using DBusPropertiesMap =
    boost::container::flat_map<std::string, DbusVariantType>;
using DBusInteracesMap =
    boost::container::flat_map<std::string, DBusPropertiesMap>;
using ManagedObjectType =
    std::vector<std::pair<sdbusplus::message::object_path, DBusInteracesMap>>;
using ManagedItem = std::pair<
    sdbusplus::message::object_path,
    boost::container::flat_map<
        std::string, boost::container::flat_map<std::string, DbusVariantType>>>;
using VariantType = std::variant<bool, std::string, uint64_t, uint32_t>;
using ManagedObjectsType = std::vector<std::pair<
    sdbusplus::message::object_path,
    std::vector<std::pair<std::string,
                          std::vector<std::pair<std::string, VariantType>>>>>>;
using PropertiesType = boost::container::flat_map<std::string, VariantType>;
        
namespace redfish
{

/**
 * Function reboots the BMC.
 *
 * @param[in] asyncResp - Shared pointer for completing asynchronous calls
 */
void doBMCGracefulRestart(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp);

void doBMCForceRestart(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp);
    
/**
 * ManagerResetAction class supports the POST method for the Reset (reboot)
 * action.
 */
void requestRoutesManagerResetAction(App& app);

/**
 * ManagerResetToDefaultsAction class supports POST method for factory reset
 * action.
 */
void requestRoutesManagerResetToDefaultsAction(App& app);

/**
 * ManagerResetActionInfo derived class for delivering Manager
 * ResetType AllowableValues using ResetInfo schema.
 */
void requestRoutesManagerResetActionInfo(App& app);

static constexpr const char* objectManagerIface =
    "org.freedesktop.DBus.ObjectManager";
static constexpr const char* pidConfigurationIface =
    "xyz.openbmc_project.Configuration.Pid";
static constexpr const char* pidZoneConfigurationIface =
    "xyz.openbmc_project.Configuration.Pid.Zone";
static constexpr const char* stepwiseConfigurationIface =
    "xyz.openbmc_project.Configuration.Stepwise";
static constexpr const char* thermalModeIface =
    "xyz.openbmc_project.Control.ThermalMode";

void asyncPopulatePid(const std::string& connection, const std::string& path,
                     const std::string& currentProfile,
                     const std::vector<std::string>& supportedProfiles,
                     const std::shared_ptr<bmcweb::AsyncResp>& asyncResp);

enum class CreatePIDRet
{
    fail,
    del,
    patch
};

bool
    getZonesFromJsonReq(const std::shared_ptr<bmcweb::AsyncResp>& response,
                        std::vector<nlohmann::json>& config,
                        std::vector<std::string>& zones);

const dbus::utility::ManagedItem*
    findChassis(const dbus::utility::ManagedObjectType& managedObj,
                const std::string& value, std::string& chassis);

CreatePIDRet createPidInterface(
    const std::shared_ptr<bmcweb::AsyncResp>& response, const std::string& type,
    const nlohmann::json::iterator& it, const std::string& path,
    const dbus::utility::ManagedObjectType& managedObj, bool createNewObject,
    boost::container::flat_map<std::string, dbus::utility::DbusVariantType>&
        output,
    std::string& chassis, const std::string& profile);

struct GetPIDValues : std::enable_shared_from_this<GetPIDValues>
{

    GetPIDValues(const std::shared_ptr<bmcweb::AsyncResp>& asyncRespIn) :
        asyncResp(asyncRespIn)
    {}

    void run();
    ~GetPIDValues();

    std::vector<std::string> supportedProfiles;
    std::string currentProfile;
    crow::openbmc_mapper::GetSubTreeType subtree;
    std::shared_ptr<bmcweb::AsyncResp> asyncResp;
};

struct SetPIDValues : std::enable_shared_from_this<SetPIDValues>
{

    SetPIDValues(const std::shared_ptr<bmcweb::AsyncResp>& asyncRespIn,
                 nlohmann::json& data) :
        asyncResp(asyncRespIn)
    {

        std::optional<nlohmann::json> pidControllers;
        std::optional<nlohmann::json> fanControllers;
        std::optional<nlohmann::json> fanZones;
        std::optional<nlohmann::json> stepwiseControllers;

        if (!redfish::json_util::readJson(
                data, asyncResp->res, "PidControllers", pidControllers,
                "FanControllers", fanControllers, "FanZones", fanZones,
                "StepwiseControllers", stepwiseControllers, "Profile", profile))
        {
            BMCWEB_LOG_ERROR
                << "Illegal Property "
                << data.dump(2, ' ', true,
                             nlohmann::json::error_handler_t::replace);
            return;
        }
        configuration.emplace_back("PidControllers", std::move(pidControllers));
        configuration.emplace_back("FanControllers", std::move(fanControllers));
        configuration.emplace_back("FanZones", std::move(fanZones));
        configuration.emplace_back("StepwiseControllers",
                                   std::move(stepwiseControllers));
    }

    void run()
    {
        if (asyncResp->res.result() != boost::beast::http::status::ok)
        {
            return;
        }

        std::shared_ptr<SetPIDValues> self = shared_from_this();

        // todo(james): might make sense to do a mapper call here if this
        // interface gets more traction
        crow::connections::systemBus->async_method_call(
            [self](const boost::system::error_code ec,
                   dbus::utility::ManagedObjectType& mObj) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Error communicating to Entity Manager";
                    messages::internalError(self->asyncResp->res);
                    return;
                }
                const std::array<const char*, 3> configurations = {
                    pidConfigurationIface, pidZoneConfigurationIface,
                    stepwiseConfigurationIface};

                for (const auto& [path, object] : mObj)
                {
                    for (const auto& [interface, _] : object)
                    {
                        if (std::find(configurations.begin(),
                                      configurations.end(),
                                      interface) != configurations.end())
                        {
                            self->objectCount++;
                            break;
                        }
                    }
                }
                self->managedObj = std::move(mObj);
            },
            "xyz.openbmc_project.EntityManager", "/", objectManagerIface,
            "GetManagedObjects");

        // at the same time get the profile information
        crow::connections::systemBus->async_method_call(
            [self](const boost::system::error_code ec,
                   const crow::openbmc_mapper::GetSubTreeType& subtree) {
                if (ec || subtree.empty())
                {
                    return;
                }
                if (subtree[0].second.empty())
                {
                    // invalid mapper response, should never happen
                    BMCWEB_LOG_ERROR << "SetPIDValues: Mapper Error";
                    messages::internalError(self->asyncResp->res);
                    return;
                }

                const std::string& path = subtree[0].first;
                const std::string& owner = subtree[0].second[0].first;
                crow::connections::systemBus->async_method_call(
                    [self, path, owner](
                        const boost::system::error_code ec2,
                        const boost::container::flat_map<
                            std::string, std::variant<std::vector<std::string>,
                                                      std::string>>& r) {
                        if (ec2)
                        {
                            BMCWEB_LOG_ERROR << "SetPIDValues: Can't get "
                                                "thermalModeIface "
                                             << path;
                            messages::internalError(self->asyncResp->res);
                            return;
                        }
                        const std::string* current = nullptr;
                        const std::vector<std::string>* supported = nullptr;
                        for (auto& [key, value] : r)
                        {
                            if (key == "Current")
                            {
                                current = std::get_if<std::string>(&value);
                                if (current == nullptr)
                                {
                                    BMCWEB_LOG_ERROR
                                        << "SetPIDValues: thermal mode "
                                           "iface invalid "
                                        << path;
                                    messages::internalError(
                                        self->asyncResp->res);
                                    return;
                                }
                            }
                            if (key == "Supported")
                            {
                                supported =
                                    std::get_if<std::vector<std::string>>(
                                        &value);
                                if (supported == nullptr)
                                {
                                    BMCWEB_LOG_ERROR
                                        << "SetPIDValues: thermal mode "
                                           "iface invalid"
                                        << path;
                                    messages::internalError(
                                        self->asyncResp->res);
                                    return;
                                }
                            }
                        }
                        if (current == nullptr || supported == nullptr)
                        {
                            BMCWEB_LOG_ERROR << "SetPIDValues: thermal mode "
                                                "iface invalid "
                                             << path;
                            messages::internalError(self->asyncResp->res);
                            return;
                        }
                        self->currentProfile = *current;
                        self->supportedProfiles = *supported;
                        self->profileConnection = owner;
                        self->profilePath = path;
                    },
                    owner, path, "org.freedesktop.DBus.Properties", "GetAll",
                    thermalModeIface);
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree", "/", 0,
            std::array<const char*, 1>{thermalModeIface});
    }
    ~SetPIDValues()
    {
        if (asyncResp->res.result() != boost::beast::http::status::ok)
        {
            return;
        }
        std::shared_ptr<bmcweb::AsyncResp> response = asyncResp;
        if (profile)
        {
            if (std::find(supportedProfiles.begin(), supportedProfiles.end(),
                          *profile) == supportedProfiles.end())
            {
                messages::actionParameterUnknown(response->res, "Profile",
                                                 *profile);
                return;
            }
            currentProfile = *profile;
            crow::connections::systemBus->async_method_call(
                [response](const boost::system::error_code ec) {
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR << "Error patching profile" << ec;
                        messages::internalError(response->res);
                    }
                },
                profileConnection, profilePath,
                "org.freedesktop.DBus.Properties", "Set", thermalModeIface,
                "Current", std::variant<std::string>(*profile));
        }

        for (auto& containerPair : configuration)
        {
            auto& container = containerPair.second;
            if (!container)
            {
                continue;
            }
            BMCWEB_LOG_DEBUG << *container;

            std::string& type = containerPair.first;

            for (nlohmann::json::iterator it = container->begin();
                 it != container->end(); ++it)
            {
                const auto& name = it.key();
                BMCWEB_LOG_DEBUG << "looking for " << name;

                auto pathItr =
                    std::find_if(managedObj.begin(), managedObj.end(),
                                 [&name](const auto& obj) {
                                     return boost::algorithm::ends_with(
                                         obj.first.str, "/" + name);
                                 });
                boost::container::flat_map<std::string,
                                           dbus::utility::DbusVariantType>
                    output;

                output.reserve(16); // The pid interface length

                // determines if we're patching entity-manager or
                // creating a new object
                bool createNewObject = (pathItr == managedObj.end());
                BMCWEB_LOG_DEBUG << "Found = " << !createNewObject;

                std::string iface;
                if (type == "PidControllers" || type == "FanControllers")
                {
                    iface = pidConfigurationIface;
                    if (!createNewObject &&
                        pathItr->second.find(pidConfigurationIface) ==
                            pathItr->second.end())
                    {
                        createNewObject = true;
                    }
                }
                else if (type == "FanZones")
                {
                    iface = pidZoneConfigurationIface;
                    if (!createNewObject &&
                        pathItr->second.find(pidZoneConfigurationIface) ==
                            pathItr->second.end())
                    {

                        createNewObject = true;
                    }
                }
                else if (type == "StepwiseControllers")
                {
                    iface = stepwiseConfigurationIface;
                    if (!createNewObject &&
                        pathItr->second.find(stepwiseConfigurationIface) ==
                            pathItr->second.end())
                    {
                        createNewObject = true;
                    }
                }

                if (createNewObject && it.value() == nullptr)
                {
                    // can't delete a non-existent object
                    messages::invalidObject(response->res, name);
                    continue;
                }

                std::string path;
                if (pathItr != managedObj.end())
                {
                    path = pathItr->first.str;
                }

                BMCWEB_LOG_DEBUG << "Create new = " << createNewObject << "\n";

                // arbitrary limit to avoid attacks
                constexpr const size_t controllerLimit = 500;
                if (createNewObject && objectCount >= controllerLimit)
                {
                    messages::resourceExhaustion(response->res, type);
                    continue;
                }

                output["Name"] = boost::replace_all_copy(name, "_", " ");

                std::string chassis;
                CreatePIDRet ret = createPidInterface(
                    response, type, it, path, managedObj, createNewObject,
                    output, chassis, currentProfile);
                if (ret == CreatePIDRet::fail)
                {
                    return;
                }
                if (ret == CreatePIDRet::del)
                {
                    continue;
                }

                if (!createNewObject)
                {
                    for (const auto& property : output)
                    {
                        crow::connections::systemBus->async_method_call(
                            [response,
                             propertyName{std::string(property.first)}](
                                const boost::system::error_code ec) {
                                if (ec)
                                {
                                    BMCWEB_LOG_ERROR << "Error patching "
                                                     << propertyName << ": "
                                                     << ec;
                                    messages::internalError(response->res);
                                    return;
                                }
                                messages::success(response->res);
                            },
                            "xyz.openbmc_project.EntityManager", path,
                            "org.freedesktop.DBus.Properties", "Set", iface,
                            property.first, property.second);
                    }
                }
                else
                {
                    if (chassis.empty())
                    {
                        BMCWEB_LOG_ERROR << "Failed to get chassis from config";
                        messages::invalidObject(response->res, name);
                        return;
                    }

                    bool foundChassis = false;
                    for (const auto& obj : managedObj)
                    {
                        if (boost::algorithm::ends_with(obj.first.str, chassis))
                        {
                            chassis = obj.first.str;
                            foundChassis = true;
                            break;
                        }
                    }
                    if (!foundChassis)
                    {
                        BMCWEB_LOG_ERROR << "Failed to find chassis on dbus";
                        messages::resourceMissingAtURI(
                            response->res, "/redfish/v1/Chassis/" + chassis);
                        return;
                    }

                    crow::connections::systemBus->async_method_call(
                        [response](const boost::system::error_code ec) {
                            if (ec)
                            {
                                BMCWEB_LOG_ERROR << "Error Adding Pid Object "
                                                 << ec;
                                messages::internalError(response->res);
                                return;
                            }
                            messages::success(response->res);
                        },
                        "xyz.openbmc_project.EntityManager", chassis,
                        "xyz.openbmc_project.AddObject", "AddObject", output);
                }
            }
        }
    }
    std::shared_ptr<bmcweb::AsyncResp> asyncResp;
    std::vector<std::pair<std::string, std::optional<nlohmann::json>>>
        configuration;
    std::optional<std::string> profile;
    dbus::utility::ManagedObjectType managedObj;
    std::vector<std::string> supportedProfiles;
    std::string currentProfile;
    std::string profileConnection;
    std::string profilePath;
    size_t objectCount = 0;
};

/**
 * @brief Retrieves BMC manager location data over DBus
 *
 * @param[in] aResp Shared pointer for completing asynchronous calls
 * @param[in] connectionName - service name
 * @param[in] path - object path
 * @return none
 */
inline void getLocation(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                        const std::string& connectionName,
                        const std::string& path)
{
    BMCWEB_LOG_DEBUG << "Get BMC manager Location data.";

    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                const std::variant<std::string>& property) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error for "
                                    "Location";
                messages::internalError(aResp->res);
                return;
            }

            const std::string* value = std::get_if<std::string>(&property);

            if (value == nullptr)
            {
                // illegal value
                messages::internalError(aResp->res);
                return;
            }

            aResp->res.jsonValue["Location"]["PartLocation"]["ServiceLabel"] =
                *value;
        },
        connectionName, path, "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Inventory.Decorator."
        "LocationCode",
        "LocationCode");
}
// avoid name collision systems.hpp
inline void
    managerGetLastResetTime(const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    BMCWEB_LOG_DEBUG << "Getting Manager Last Reset Time";

    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                std::variant<uint64_t>& lastResetTime) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "D-BUS response error " << ec;
                return;
            }

            const uint64_t* lastResetTimePtr =
                std::get_if<uint64_t>(&lastResetTime);

            if (!lastResetTimePtr)
            {
                messages::internalError(aResp->res);
                return;
            }
            // LastRebootTime is epoch time, in milliseconds
            // https://github.com/openbmc/phosphor-dbus-interfaces/blob/7f9a128eb9296e926422ddc312c148b625890bb6/xyz/openbmc_project/State/BMC.interface.yaml#L19
            time_t lastResetTimeStamp =
                static_cast<time_t>(*lastResetTimePtr / 1000);

            // Convert to ISO 8601 standard
            aResp->res.jsonValue["LastResetTime"] =
                crow::utility::getDateTime(lastResetTimeStamp);
        },
        "xyz.openbmc_project.State.BMC", "/xyz/openbmc_project/state/bmc0",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.State.BMC", "LastRebootTime");
}

/**
 * @brief Set the running firmware image
 *
 * @param[i,o] aResp - Async response object
 * @param[i] runningFirmwareTarget - Image to make the running image
 *
 * @return void
 */
inline void
    setActiveFirmwareImage(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                           const std::string& runningFirmwareTarget)
{
    // Get the Id from /redfish/v1/UpdateService/FirmwareInventory/<Id>
    std::string::size_type idPos = runningFirmwareTarget.rfind('/');
    if (idPos == std::string::npos)
    {
        messages::propertyValueNotInList(aResp->res, runningFirmwareTarget,
                                         "@odata.id");
        BMCWEB_LOG_DEBUG << "Can't parse firmware ID!";
        return;
    }
    idPos++;
    if (idPos >= runningFirmwareTarget.size())
    {
        messages::propertyValueNotInList(aResp->res, runningFirmwareTarget,
                                         "@odata.id");
        BMCWEB_LOG_DEBUG << "Invalid firmware ID.";
        return;
    }
    std::string firmwareId = runningFirmwareTarget.substr(idPos);

    // Make sure the image is valid before setting priority
    crow::connections::systemBus->async_method_call(
        [aResp, firmwareId, runningFirmwareTarget](
            const boost::system::error_code ec, ManagedObjectType& subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "D-Bus response error getting objects.";
                messages::internalError(aResp->res);
                return;
            }

            if (subtree.size() == 0)
            {
                BMCWEB_LOG_DEBUG << "Can't find image!";
                messages::internalError(aResp->res);
                return;
            }

            bool foundImage = false;
            for (auto& object : subtree)
            {
                const std::string& path =
                    static_cast<const std::string&>(object.first);
                std::size_t idPos2 = path.rfind('/');

                if (idPos2 == std::string::npos)
                {
                    continue;
                }

                idPos2++;
                if (idPos2 >= path.size())
                {
                    continue;
                }

                if (path.substr(idPos2) == firmwareId)
                {
                    foundImage = true;
                    break;
                }
            }

            if (!foundImage)
            {
                messages::propertyValueNotInList(
                    aResp->res, runningFirmwareTarget, "@odata.id");
                BMCWEB_LOG_DEBUG << "Invalid firmware ID.";
                return;
            }

            BMCWEB_LOG_DEBUG
                << "Setting firmware version " + firmwareId + " to priority 0.";

            // Only support Immediate
            // An addition could be a Redfish Setting like
            // ActiveSoftwareImageApplyTime and support OnReset
            crow::connections::systemBus->async_method_call(
                [aResp](const boost::system::error_code ec) {
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG << "D-Bus response error setting.";
                        messages::internalError(aResp->res);
                        return;
                    }
                    doBMCGracefulRestart(aResp);
                },

                "xyz.openbmc_project.Software.BMC.Updater",
                "/xyz/openbmc_project/software/" + firmwareId,
                "org.freedesktop.DBus.Properties", "Set",
                "xyz.openbmc_project.Software.RedundancyPriority", "Priority",
                std::variant<uint8_t>(static_cast<uint8_t>(0)));
        },
        "xyz.openbmc_project.Software.BMC.Updater",
        "/xyz/openbmc_project/software", "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects");
}

inline void setDateTime(std::shared_ptr<bmcweb::AsyncResp> aResp,
                        std::string datetime)
{
    BMCWEB_LOG_DEBUG << "Set date time: " << datetime;

    std::stringstream stream(datetime);
    // Convert from ISO 8601 to boost local_time
    // (BMC only has time in UTC)
    boost::posix_time::ptime posixTime;
    boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
    // Facet gets deleted with the stringsteam
    auto ifc = std::make_unique<boost::local_time::local_time_input_facet>(
        "%Y-%m-%d %H:%M:%S%F %ZP");
    stream.imbue(std::locale(stream.getloc(), ifc.release()));

    boost::local_time::local_date_time ldt(boost::local_time::not_a_date_time);

    if (stream >> ldt)
    {
        posixTime = ldt.utc_time();
        boost::posix_time::time_duration dur = posixTime - epoch;
        uint64_t durMicroSecs = static_cast<uint64_t>(dur.total_microseconds());
        crow::connections::systemBus->async_method_call(
            [aResp{std::move(aResp)}, datetime{std::move(datetime)}](
                const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG << "Failed to set elapsed time. "
                                        "DBUS response error "
                                     << ec;
                    messages::internalError(aResp->res);
                    return;
                }
                aResp->res.jsonValue["DateTime"] = datetime;
            },
            "xyz.openbmc_project.Time.Manager", "/xyz/openbmc_project/time/bmc",
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Time.EpochTime", "Elapsed",
            std::variant<uint64_t>(durMicroSecs));
    }
    else
    {
        messages::propertyValueFormatError(aResp->res, datetime, "DateTime");
        return;
    }
}

void requestRoutesManager(App& app);
void requestRoutesManagerCollection(App& app);

} // namespace redfish
