#include "../../include/dbus_singleton.hpp"
#include "../../include/persistent_data.hpp"
#include "../../http/logging.hpp"
#include "../include/utils/json_utils.hpp"
#include "../../http/app_class_decl.hpp"
using crow::App;
#include <boost/container/flat_set.hpp>
#include "managers.hpp"

namespace redfish {

void doBMCGracefulRestart(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    const char* processName = "xyz.openbmc_project.State.BMC";
    const char* objectPath = "/xyz/openbmc_project/state/bmc0";
    const char* interfaceName = "xyz.openbmc_project.State.BMC";
    const std::string& propertyValue =
        "xyz.openbmc_project.State.BMC.Transition.Reboot";
    const char* destProperty = "RequestedBMCTransition";

    // Create the D-Bus variant for D-Bus call.
    VariantType dbusPropertyValue(propertyValue);

    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec) {
            // Use "Set" method to set the property value.
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "[Set] Bad D-Bus request error: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }

            messages::success(asyncResp->res);
        },
        processName, objectPath, "org.freedesktop.DBus.Properties", "Set",
        interfaceName, destProperty, dbusPropertyValue);
}

void doBMCForceRestart(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    const char* processName = "xyz.openbmc_project.State.BMC";
    const char* objectPath = "/xyz/openbmc_project/state/bmc0";
    const char* interfaceName = "xyz.openbmc_project.State.BMC";
    const std::string& propertyValue =
        "xyz.openbmc_project.State.BMC.Transition.HardReboot";
    const char* destProperty = "RequestedBMCTransition";

    // Create the D-Bus variant for D-Bus call.
    VariantType dbusPropertyValue(propertyValue);

    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec) {
            // Use "Set" method to set the property value.
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "[Set] Bad D-Bus request error: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }

            messages::success(asyncResp->res);
        },
        processName, objectPath, "org.freedesktop.DBus.Properties", "Set",
        interfaceName, destProperty, dbusPropertyValue);
}

bool getZonesFromJsonReq(const std::shared_ptr<bmcweb::AsyncResp>& response,
                        std::vector<nlohmann::json>& config,
                        std::vector<std::string>& zones)
{
    if (config.empty())
    {
        BMCWEB_LOG_ERROR << "Empty Zones";
        messages::propertyValueFormatError(response->res,
                                           nlohmann::json::array(), "Zones");
        return false;
    }
    for (auto& odata : config)
    {
        std::string path;
        if (!redfish::json_util::readJson(odata, response->res, "@odata.id",
                                          path))
        {
            return false;
        }
        std::string input;

        // 8 below comes from
        // /redfish/v1/Managers/bmc#/Oem/OpenBmc/Fan/FanZones/Left
        //     0    1     2      3    4    5      6     7      8
        if (!dbus::utility::getNthStringFromPath(path, 8, input))
        {
            BMCWEB_LOG_ERROR << "Got invalid path " << path;
            BMCWEB_LOG_ERROR << "Illegal Type Zones";
            messages::propertyValueFormatError(response->res, odata.dump(),
                                               "Zones");
            return false;
        }
        boost::replace_all(input, "_", " ");
        zones.emplace_back(std::move(input));
    }
    return true;
}

const dbus::utility::ManagedItem*
    findChassis(const dbus::utility::ManagedObjectType& managedObj,
                const std::string& value, std::string& chassis)
{
    BMCWEB_LOG_DEBUG << "Find Chassis: " << value << "\n";

    std::string escaped = boost::replace_all_copy(value, " ", "_");
    escaped = "/" + escaped;
    auto it = std::find_if(
        managedObj.begin(), managedObj.end(), [&escaped](const auto& obj) {
            if (boost::algorithm::ends_with(obj.first.str, escaped))
            {
                BMCWEB_LOG_DEBUG << "Matched " << obj.first.str << "\n";
                return true;
            }
            return false;
        });

    if (it == managedObj.end())
    {
        return nullptr;
    }
    // 5 comes from <chassis-name> being the 5th element
    // /xyz/openbmc_project/inventory/system/chassis/<chassis-name>
    if (dbus::utility::getNthStringFromPath(it->first.str, 5, chassis))
    {
        return &(*it);
    }

    return nullptr;
}

CreatePIDRet createPidInterface(
    const std::shared_ptr<bmcweb::AsyncResp>& response, const std::string& type,
    const nlohmann::json::iterator& it, const std::string& path,
    const dbus::utility::ManagedObjectType& managedObj, bool createNewObject,
    boost::container::flat_map<std::string, dbus::utility::DbusVariantType>&
        output,
    std::string& chassis, const std::string& profile)
{
    // common deleter
    if (it.value() == nullptr)
    {
        std::string iface;
        if (type == "PidControllers" || type == "FanControllers")
        {
            iface = pidConfigurationIface;
        }
        else if (type == "FanZones")
        {
            iface = pidZoneConfigurationIface;
        }
        else if (type == "StepwiseControllers")
        {
            iface = stepwiseConfigurationIface;
        }
        else
        {
            BMCWEB_LOG_ERROR << "Illegal Type " << type;
            messages::propertyUnknown(response->res, type);
            return CreatePIDRet::fail;
        }

        BMCWEB_LOG_DEBUG << "del " << path << " " << iface << "\n";
        // delete interface
        crow::connections::systemBus->async_method_call(
            [response, path](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Error patching " << path << ": " << ec;
                    messages::internalError(response->res);
                    return;
                }
                messages::success(response->res);
            },
            "xyz.openbmc_project.EntityManager", path, iface, "Delete");
        return CreatePIDRet::del;
    }

    const dbus::utility::ManagedItem* managedItem = nullptr;
    if (!createNewObject)
    {
        // if we aren't creating a new object, we should be able to find it on
        // d-bus
        managedItem = findChassis(managedObj, it.key(), chassis);
        if (managedItem == nullptr)
        {
            BMCWEB_LOG_ERROR << "Failed to get chassis from config patch";
            messages::invalidObject(response->res, it.key());
            return CreatePIDRet::fail;
        }
    }

    if (profile.size() &&
        (type == "PidControllers" || type == "FanControllers" ||
         type == "StepwiseControllers"))
    {
        if (managedItem == nullptr)
        {
            output["Profiles"] = std::vector<std::string>{profile};
        }
        else
        {
            std::string interface;
            if (type == "StepwiseControllers")
            {
                interface = stepwiseConfigurationIface;
            }
            else
            {
                interface = pidConfigurationIface;
            }
            auto findConfig = managedItem->second.find(interface);
            if (findConfig == managedItem->second.end())
            {
                BMCWEB_LOG_ERROR
                    << "Failed to find interface in managed object";
                messages::internalError(response->res);
                return CreatePIDRet::fail;
            }
            auto findProfiles = findConfig->second.find("Profiles");
            if (findProfiles != findConfig->second.end())
            {
                const std::vector<std::string>* curProfiles =
                    std::get_if<std::vector<std::string>>(
                        &(findProfiles->second));
                if (curProfiles == nullptr)
                {
                    BMCWEB_LOG_ERROR << "Illegal profiles in managed object";
                    messages::internalError(response->res);
                    return CreatePIDRet::fail;
                }
                if (std::find(curProfiles->begin(), curProfiles->end(),
                              profile) == curProfiles->end())
                {
                    std::vector<std::string> newProfiles = *curProfiles;
                    newProfiles.push_back(profile);
                    output["Profiles"] = newProfiles;
                }
            }
        }
    }

    if (type == "PidControllers" || type == "FanControllers")
    {
        if (createNewObject)
        {
            output["Class"] = type == "PidControllers" ? std::string("temp")
                                                       : std::string("fan");
            output["Type"] = std::string("Pid");
        }

        std::optional<std::vector<nlohmann::json>> zones;
        std::optional<std::vector<std::string>> inputs;
        std::optional<std::vector<std::string>> outputs;
        std::map<std::string, std::optional<double>> doubles;
        std::optional<std::string> setpointOffset;
        if (!redfish::json_util::readJson(
                it.value(), response->res, "Inputs", inputs, "Outputs", outputs,
                "Zones", zones, "FFGainCoefficient",
                doubles["FFGainCoefficient"], "FFOffCoefficient",
                doubles["FFOffCoefficient"], "ICoefficient",
                doubles["ICoefficient"], "ILimitMax", doubles["ILimitMax"],
                "ILimitMin", doubles["ILimitMin"], "OutLimitMax",
                doubles["OutLimitMax"], "OutLimitMin", doubles["OutLimitMin"],
                "PCoefficient", doubles["PCoefficient"], "SetPoint",
                doubles["SetPoint"], "SetPointOffset", setpointOffset,
                "SlewNeg", doubles["SlewNeg"], "SlewPos", doubles["SlewPos"],
                "PositiveHysteresis", doubles["PositiveHysteresis"],
                "NegativeHysteresis", doubles["NegativeHysteresis"]))
        {
            BMCWEB_LOG_ERROR
                << "Illegal Property "
                << it.value().dump(2, ' ', true,
                                   nlohmann::json::error_handler_t::replace);
            return CreatePIDRet::fail;
        }
        if (zones)
        {
            std::vector<std::string> zonesStr;
            if (!getZonesFromJsonReq(response, *zones, zonesStr))
            {
                BMCWEB_LOG_ERROR << "Illegal Zones";
                return CreatePIDRet::fail;
            }
            if (chassis.empty() &&
                !findChassis(managedObj, zonesStr[0], chassis))
            {
                BMCWEB_LOG_ERROR << "Failed to get chassis from config patch";
                messages::invalidObject(response->res, it.key());
                return CreatePIDRet::fail;
            }

            output["Zones"] = std::move(zonesStr);
        }
        if (inputs || outputs)
        {
            std::array<std::optional<std::vector<std::string>>*, 2> containers =
                {&inputs, &outputs};
            size_t index = 0;
            for (const auto& containerPtr : containers)
            {
                std::optional<std::vector<std::string>>& container =
                    *containerPtr;
                if (!container)
                {
                    index++;
                    continue;
                }

                for (std::string& value : *container)
                {
                    boost::replace_all(value, "_", " ");
                }
                std::string key;
                if (index == 0)
                {
                    key = "Inputs";
                }
                else
                {
                    key = "Outputs";
                }
                output[key] = *container;
                index++;
            }
        }

        if (setpointOffset)
        {
            // translate between redfish and dbus names
            if (*setpointOffset == "UpperThresholdNonCritical")
            {
                output["SetPointOffset"] = std::string("WarningLow");
            }
            else if (*setpointOffset == "LowerThresholdNonCritical")
            {
                output["SetPointOffset"] = std::string("WarningHigh");
            }
            else if (*setpointOffset == "LowerThresholdCritical")
            {
                output["SetPointOffset"] = std::string("CriticalLow");
            }
            else if (*setpointOffset == "UpperThresholdCritical")
            {
                output["SetPointOffset"] = std::string("CriticalHigh");
            }
            else
            {
                BMCWEB_LOG_ERROR << "Invalid setpointoffset "
                                 << *setpointOffset;
                messages::invalidObject(response->res, it.key());
                return CreatePIDRet::fail;
            }
        }

        // doubles
        for (const auto& pairs : doubles)
        {
            if (!pairs.second)
            {
                continue;
            }
            BMCWEB_LOG_DEBUG << pairs.first << " = " << *pairs.second;
            output[pairs.first] = *(pairs.second);
        }
    }

    else if (type == "FanZones")
    {
        output["Type"] = std::string("Pid.Zone");

        std::optional<nlohmann::json> chassisContainer;
        std::optional<double> failSafePercent;
        std::optional<double> minThermalOutput;
        if (!redfish::json_util::readJson(it.value(), response->res, "Chassis",
                                          chassisContainer, "FailSafePercent",
                                          failSafePercent, "MinThermalOutput",
                                          minThermalOutput))
        {
            BMCWEB_LOG_ERROR
                << "Illegal Property "
                << it.value().dump(2, ' ', true,
                                   nlohmann::json::error_handler_t::replace);
            return CreatePIDRet::fail;
        }

        if (chassisContainer)
        {

            std::string chassisId;
            if (!redfish::json_util::readJson(*chassisContainer, response->res,
                                              "@odata.id", chassisId))
            {
                BMCWEB_LOG_ERROR
                    << "Illegal Property "
                    << chassisContainer->dump(
                           2, ' ', true,
                           nlohmann::json::error_handler_t::replace);
                return CreatePIDRet::fail;
            }

            // /redfish/v1/chassis/chassis_name/
            if (!dbus::utility::getNthStringFromPath(chassisId, 3, chassis))
            {
                BMCWEB_LOG_ERROR << "Got invalid path " << chassisId;
                messages::invalidObject(response->res, chassisId);
                return CreatePIDRet::fail;
            }
        }
        if (minThermalOutput)
        {
            output["MinThermalOutput"] = *minThermalOutput;
        }
        if (failSafePercent)
        {
            output["FailSafePercent"] = *failSafePercent;
        }
    }
    else if (type == "StepwiseControllers")
    {
        output["Type"] = std::string("Stepwise");

        std::optional<std::vector<nlohmann::json>> zones;
        std::optional<std::vector<nlohmann::json>> steps;
        std::optional<std::vector<std::string>> inputs;
        std::optional<double> positiveHysteresis;
        std::optional<double> negativeHysteresis;
        std::optional<std::string> direction; // upper clipping curve vs lower
        if (!redfish::json_util::readJson(
                it.value(), response->res, "Zones", zones, "Steps", steps,
                "Inputs", inputs, "PositiveHysteresis", positiveHysteresis,
                "NegativeHysteresis", negativeHysteresis, "Direction",
                direction))
        {
            BMCWEB_LOG_ERROR
                << "Illegal Property "
                << it.value().dump(2, ' ', true,
                                   nlohmann::json::error_handler_t::replace);
            return CreatePIDRet::fail;
        }

        if (zones)
        {
            std::vector<std::string> zonesStrs;
            if (!getZonesFromJsonReq(response, *zones, zonesStrs))
            {
                BMCWEB_LOG_ERROR << "Illegal Zones";
                return CreatePIDRet::fail;
            }
            if (chassis.empty() &&
                !findChassis(managedObj, zonesStrs[0], chassis))
            {
                BMCWEB_LOG_ERROR << "Failed to get chassis from config patch";
                messages::invalidObject(response->res, it.key());
                return CreatePIDRet::fail;
            }
            output["Zones"] = std::move(zonesStrs);
        }
        if (steps)
        {
            std::vector<double> readings;
            std::vector<double> outputs;
            for (auto& step : *steps)
            {
                double target;
                double out;

                if (!redfish::json_util::readJson(step, response->res, "Target",
                                                  target, "Output", out))
                {
                    BMCWEB_LOG_ERROR
                        << "Illegal Property "
                        << it.value().dump(
                               2, ' ', true,
                               nlohmann::json::error_handler_t::replace);
                    return CreatePIDRet::fail;
                }
                readings.emplace_back(target);
                outputs.emplace_back(out);
            }
            output["Reading"] = std::move(readings);
            output["Output"] = std::move(outputs);
        }
        if (inputs)
        {
            for (std::string& value : *inputs)
            {
                boost::replace_all(value, "_", " ");
            }
            output["Inputs"] = std::move(*inputs);
        }
        if (negativeHysteresis)
        {
            output["NegativeHysteresis"] = *negativeHysteresis;
        }
        if (positiveHysteresis)
        {
            output["PositiveHysteresis"] = *positiveHysteresis;
        }
        if (direction)
        {
            constexpr const std::array<const char*, 2> allowedDirections = {
                "Ceiling", "Floor"};
            if (std::find(allowedDirections.begin(), allowedDirections.end(),
                          *direction) == allowedDirections.end())
            {
                messages::propertyValueTypeError(response->res, "Direction",
                                                 *direction);
                return CreatePIDRet::fail;
            }
            output["Class"] = *direction;
        }
    }
    else
    {
        BMCWEB_LOG_ERROR << "Illegal Type " << type;
        messages::propertyUnknown(response->res, type);
        return CreatePIDRet::fail;
    }
    return CreatePIDRet::patch;
}

void GetPIDValues::run()
{
    std::shared_ptr<GetPIDValues> self = shared_from_this();

    // get all configurations
    crow::connections::systemBus->async_method_call(
        [self](const boost::system::error_code ec,
                const crow::openbmc_mapper::GetSubTreeType& subtreeLocal) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << ec;
                messages::internalError(self->asyncResp->res);
                return;
            }
            self->subtree = subtreeLocal;
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", "/", 0,
        std::array<const char*, 4>{
            pidConfigurationIface, pidZoneConfigurationIface,
            objectManagerIface, stepwiseConfigurationIface});

    // at the same time get the selected profile
    crow::connections::systemBus->async_method_call(
        [self](const boost::system::error_code ec,
                const crow::openbmc_mapper::GetSubTreeType& subtreeLocal) {
            if (ec || subtreeLocal.empty())
            {
                return;
            }
            if (subtreeLocal[0].second.size() != 1)
            {
                // invalid mapper response, should never happen
                BMCWEB_LOG_ERROR << "GetPIDValues: Mapper Error";
                messages::internalError(self->asyncResp->res);
                return;
            }

            const std::string& path = subtreeLocal[0].first;
            const std::string& owner = subtreeLocal[0].second[0].first;
            crow::connections::systemBus->async_method_call(
                [path, owner, self](
                    const boost::system::error_code ec2,
                    const boost::container::flat_map<
                        std::string, std::variant<std::vector<std::string>,
                                                    std::string>>& resp) {
                    if (ec2)
                    {
                        BMCWEB_LOG_ERROR << "GetPIDValues: Can't get "
                                            "thermalModeIface "
                                            << path;
                        messages::internalError(self->asyncResp->res);
                        return;
                    }
                    const std::string* current = nullptr;
                    const std::vector<std::string>* supported = nullptr;
                    for (auto& [key, value] : resp)
                    {
                        if (key == "Current")
                        {
                            current = std::get_if<std::string>(&value);
                            if (current == nullptr)
                            {
                                BMCWEB_LOG_ERROR
                                    << "GetPIDValues: thermal mode "
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
                                    << "GetPIDValues: thermal mode "
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
                        BMCWEB_LOG_ERROR << "GetPIDValues: thermal mode "
                                            "iface invalid "
                                            << path;
                        messages::internalError(self->asyncResp->res);
                        return;
                    }
                    self->currentProfile = *current;
                    self->supportedProfiles = *supported;
                },
                owner, path, "org.freedesktop.DBus.Properties", "GetAll",
                thermalModeIface);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", "/", 0,
        std::array<const char*, 1>{thermalModeIface});
}

GetPIDValues::~GetPIDValues()
{
    if (asyncResp->res.result() != boost::beast::http::status::ok)
    {
        return;
    }
    // create map of <connection, path to objMgr>>
    boost::container::flat_map<std::string, std::string> objectMgrPaths;
    boost::container::flat_set<std::string> calledConnections;
    for (const auto& pathGroup : subtree)
    {
        for (const auto& connectionGroup : pathGroup.second)
        {
            auto findConnection =
                calledConnections.find(connectionGroup.first);
            if (findConnection != calledConnections.end())
            {
                break;
            }
            for (const std::string& interface : connectionGroup.second)
            {
                if (interface == objectManagerIface)
                {
                    objectMgrPaths[connectionGroup.first] = pathGroup.first;
                }
                // this list is alphabetical, so we
                // should have found the objMgr by now
                if (interface == pidConfigurationIface ||
                    interface == pidZoneConfigurationIface ||
                    interface == stepwiseConfigurationIface)
                {
                    auto findObjMgr =
                        objectMgrPaths.find(connectionGroup.first);
                    if (findObjMgr == objectMgrPaths.end())
                    {
                        BMCWEB_LOG_DEBUG << connectionGroup.first
                                            << "Has no Object Manager";
                        continue;
                    }

                    calledConnections.insert(connectionGroup.first);

                    asyncPopulatePid(findObjMgr->first, findObjMgr->second,
                                        currentProfile, supportedProfiles,
                                        asyncResp);
                    break;
                }
            }
        }
    }
}

void asyncPopulatePid(const std::string& connection, const std::string& path,
                     const std::string& currentProfile,
                     const std::vector<std::string>& supportedProfiles,
                     const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, currentProfile, supportedProfiles](
            const boost::system::error_code ec,
            const dbus::utility::ManagedObjectType& managedObj) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << ec;
                asyncResp->res.jsonValue.clear();
                messages::internalError(asyncResp->res);
                return;
            }
            nlohmann::json& configRoot =
                asyncResp->res.jsonValue["Oem"]["OpenBmc"]["Fan"];
            nlohmann::json& fans = configRoot["FanControllers"];
            fans["@odata.type"] = "#OemManager.FanControllers";
            fans["@odata.id"] = "/redfish/v1/Managers/bmc#/Oem/OpenBmc/"
                                "Fan/FanControllers";

            nlohmann::json& pids = configRoot["PidControllers"];
            pids["@odata.type"] = "#OemManager.PidControllers";
            pids["@odata.id"] =
                "/redfish/v1/Managers/bmc#/Oem/OpenBmc/Fan/PidControllers";

            nlohmann::json& stepwise = configRoot["StepwiseControllers"];
            stepwise["@odata.type"] = "#OemManager.StepwiseControllers";
            stepwise["@odata.id"] =
                "/redfish/v1/Managers/bmc#/Oem/OpenBmc/Fan/StepwiseControllers";

            nlohmann::json& zones = configRoot["FanZones"];
            zones["@odata.id"] =
                "/redfish/v1/Managers/bmc#/Oem/OpenBmc/Fan/FanZones";
            zones["@odata.type"] = "#OemManager.FanZones";
            configRoot["@odata.id"] =
                "/redfish/v1/Managers/bmc#/Oem/OpenBmc/Fan";
            configRoot["@odata.type"] = "#OemManager.Fan";
            configRoot["Profile@Redfish.AllowableValues"] = supportedProfiles;

            if (!currentProfile.empty())
            {
                configRoot["Profile"] = currentProfile;
            }
            BMCWEB_LOG_ERROR << "profile = " << currentProfile << " !";

            for (const auto& pathPair : managedObj)
            {
                for (const auto& intfPair : pathPair.second)
                {
                    if (intfPair.first != pidConfigurationIface &&
                        intfPair.first != pidZoneConfigurationIface &&
                        intfPair.first != stepwiseConfigurationIface)
                    {
                        continue;
                    }
                    auto findName = intfPair.second.find("Name");
                    if (findName == intfPair.second.end())
                    {
                        BMCWEB_LOG_ERROR << "Pid Field missing Name";
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    const std::string* namePtr =
                        std::get_if<std::string>(&findName->second);
                    if (namePtr == nullptr)
                    {
                        BMCWEB_LOG_ERROR << "Pid Name Field illegal";
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    std::string name = *namePtr;
                    dbus::utility::escapePathForDbus(name);

                    auto findProfiles = intfPair.second.find("Profiles");
                    if (findProfiles != intfPair.second.end())
                    {
                        const std::vector<std::string>* profiles =
                            std::get_if<std::vector<std::string>>(
                                &findProfiles->second);
                        if (profiles == nullptr)
                        {
                            BMCWEB_LOG_ERROR << "Pid Profiles Field illegal";
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        if (std::find(profiles->begin(), profiles->end(),
                                      currentProfile) == profiles->end())
                        {
                            BMCWEB_LOG_INFO
                                << name << " not supported in current profile";
                            continue;
                        }
                    }
                    nlohmann::json* config = nullptr;

                    const std::string* classPtr = nullptr;
                    auto findClass = intfPair.second.find("Class");
                    if (findClass != intfPair.second.end())
                    {
                        classPtr = std::get_if<std::string>(&findClass->second);
                    }

                    if (intfPair.first == pidZoneConfigurationIface)
                    {
                        std::string chassis;
                        if (!dbus::utility::getNthStringFromPath(
                                pathPair.first.str, 5, chassis))
                        {
                            chassis = "#IllegalValue";
                        }
                        nlohmann::json& zone = zones[name];
                        zone["Chassis"] = {
                            {"@odata.id", "/redfish/v1/Chassis/" + chassis}};
                        zone["@odata.id"] = "/redfish/v1/Managers/bmc#/Oem/"
                                            "OpenBmc/Fan/FanZones/" +
                                            name;
                        zone["@odata.type"] = "#OemManager.FanZone";
                        config = &zone;
                    }

                    else if (intfPair.first == stepwiseConfigurationIface)
                    {
                        if (classPtr == nullptr)
                        {
                            BMCWEB_LOG_ERROR << "Pid Class Field illegal";
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        nlohmann::json& controller = stepwise[name];
                        config = &controller;

                        controller["@odata.id"] =
                            "/redfish/v1/Managers/bmc#/Oem/"
                            "OpenBmc/Fan/StepwiseControllers/" +
                            name;
                        controller["@odata.type"] =
                            "#OemManager.StepwiseController";

                        controller["Direction"] = *classPtr;
                    }

                    // pid and fans are off the same configuration
                    else if (intfPair.first == pidConfigurationIface)
                    {

                        if (classPtr == nullptr)
                        {
                            BMCWEB_LOG_ERROR << "Pid Class Field illegal";
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        bool isFan = *classPtr == "fan";
                        nlohmann::json& element =
                            isFan ? fans[name] : pids[name];
                        config = &element;
                        if (isFan)
                        {
                            element["@odata.id"] =
                                "/redfish/v1/Managers/bmc#/Oem/"
                                "OpenBmc/Fan/FanControllers/" +
                                name;
                            element["@odata.type"] =
                                "#OemManager.FanController";
                        }
                        else
                        {
                            element["@odata.id"] =
                                "/redfish/v1/Managers/bmc#/Oem/"
                                "OpenBmc/Fan/PidControllers/" +
                                name;
                            element["@odata.type"] =
                                "#OemManager.PidController";
                        }
                    }
                    else
                    {
                        BMCWEB_LOG_ERROR << "Unexpected configuration";
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    // used for making maps out of 2 vectors
                    const std::vector<double>* keys = nullptr;
                    const std::vector<double>* values = nullptr;

                    for (const auto& propertyPair : intfPair.second)
                    {
                        if (propertyPair.first == "Type" ||
                            propertyPair.first == "Class" ||
                            propertyPair.first == "Name")
                        {
                            continue;
                        }

                        // zones
                        if (intfPair.first == pidZoneConfigurationIface)
                        {
                            const double* ptr =
                                std::get_if<double>(&propertyPair.second);
                            if (ptr == nullptr)
                            {
                                BMCWEB_LOG_ERROR << "Field Illegal "
                                                 << propertyPair.first;
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            (*config)[propertyPair.first] = *ptr;
                        }

                        if (intfPair.first == stepwiseConfigurationIface)
                        {
                            if (propertyPair.first == "Reading" ||
                                propertyPair.first == "Output")
                            {
                                const std::vector<double>* ptr =
                                    std::get_if<std::vector<double>>(
                                        &propertyPair.second);

                                if (ptr == nullptr)
                                {
                                    BMCWEB_LOG_ERROR << "Field Illegal "
                                                     << propertyPair.first;
                                    messages::internalError(asyncResp->res);
                                    return;
                                }

                                if (propertyPair.first == "Reading")
                                {
                                    keys = ptr;
                                }
                                else
                                {
                                    values = ptr;
                                }
                                if (keys && values)
                                {
                                    if (keys->size() != values->size())
                                    {
                                        BMCWEB_LOG_ERROR
                                            << "Reading and Output size don't "
                                               "match ";
                                        messages::internalError(asyncResp->res);
                                        return;
                                    }
                                    nlohmann::json& steps = (*config)["Steps"];
                                    steps = nlohmann::json::array();
                                    for (size_t ii = 0; ii < keys->size(); ii++)
                                    {
                                        steps.push_back(
                                            {{"Target", (*keys)[ii]},
                                             {"Output", (*values)[ii]}});
                                    }
                                }
                            }
                            if (propertyPair.first == "NegativeHysteresis" ||
                                propertyPair.first == "PositiveHysteresis")
                            {
                                const double* ptr =
                                    std::get_if<double>(&propertyPair.second);
                                if (ptr == nullptr)
                                {
                                    BMCWEB_LOG_ERROR << "Field Illegal "
                                                     << propertyPair.first;
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                (*config)[propertyPair.first] = *ptr;
                            }
                        }

                        // pid and fans are off the same configuration
                        if (intfPair.first == pidConfigurationIface ||
                            intfPair.first == stepwiseConfigurationIface)
                        {

                            if (propertyPair.first == "Zones")
                            {
                                const std::vector<std::string>* inputs =
                                    std::get_if<std::vector<std::string>>(
                                        &propertyPair.second);

                                if (inputs == nullptr)
                                {
                                    BMCWEB_LOG_ERROR
                                        << "Zones Pid Field Illegal";
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                auto& data = (*config)[propertyPair.first];
                                data = nlohmann::json::array();
                                for (std::string itemCopy : *inputs)
                                {
                                    dbus::utility::escapePathForDbus(itemCopy);
                                    data.push_back(
                                        {{"@odata.id",
                                          "/redfish/v1/Managers/bmc#/Oem/"
                                          "OpenBmc/Fan/FanZones/" +
                                              itemCopy}});
                                }
                            }
                            // todo(james): may never happen, but this
                            // assumes configuration data referenced in the
                            // PID config is provided by the same daemon, we
                            // could add another loop to cover all cases,
                            // but I'm okay kicking this can down the road a
                            // bit

                            else if (propertyPair.first == "Inputs" ||
                                     propertyPair.first == "Outputs")
                            {
                                auto& data = (*config)[propertyPair.first];
                                const std::vector<std::string>* inputs =
                                    std::get_if<std::vector<std::string>>(
                                        &propertyPair.second);

                                if (inputs == nullptr)
                                {
                                    BMCWEB_LOG_ERROR << "Field Illegal "
                                                     << propertyPair.first;
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                data = *inputs;
                            }
                            else if (propertyPair.first == "SetPointOffset")
                            {
                                const std::string* ptr =
                                    std::get_if<std::string>(
                                        &propertyPair.second);

                                if (ptr == nullptr)
                                {
                                    BMCWEB_LOG_ERROR << "Field Illegal "
                                                     << propertyPair.first;
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                // translate from dbus to redfish
                                if (*ptr == "WarningHigh")
                                {
                                    (*config)["SetPointOffset"] =
                                        "UpperThresholdNonCritical";
                                }
                                else if (*ptr == "WarningLow")
                                {
                                    (*config)["SetPointOffset"] =
                                        "LowerThresholdNonCritical";
                                }
                                else if (*ptr == "CriticalHigh")
                                {
                                    (*config)["SetPointOffset"] =
                                        "UpperThresholdCritical";
                                }
                                else if (*ptr == "CriticalLow")
                                {
                                    (*config)["SetPointOffset"] =
                                        "LowerThresholdCritical";
                                }
                                else
                                {
                                    BMCWEB_LOG_ERROR << "Value Illegal "
                                                     << *ptr;
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                            }
                            // doubles
                            else if (propertyPair.first ==
                                         "FFGainCoefficient" ||
                                     propertyPair.first == "FFOffCoefficient" ||
                                     propertyPair.first == "ICoefficient" ||
                                     propertyPair.first == "ILimitMax" ||
                                     propertyPair.first == "ILimitMin" ||
                                     propertyPair.first ==
                                         "PositiveHysteresis" ||
                                     propertyPair.first ==
                                         "NegativeHysteresis" ||
                                     propertyPair.first == "OutLimitMax" ||
                                     propertyPair.first == "OutLimitMin" ||
                                     propertyPair.first == "PCoefficient" ||
                                     propertyPair.first == "SetPoint" ||
                                     propertyPair.first == "SlewNeg" ||
                                     propertyPair.first == "SlewPos")
                            {
                                const double* ptr =
                                    std::get_if<double>(&propertyPair.second);
                                if (ptr == nullptr)
                                {
                                    BMCWEB_LOG_ERROR << "Field Illegal "
                                                     << propertyPair.first;
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                (*config)[propertyPair.first] = *ptr;
                            }
                        }
                    }
                }
            }
        },
        connection, path, objectManagerIface, "GetManagedObjects");
}

void requestRoutesManagerResetAction(App& app)
{
    /**
     * Function handles POST method request.
     * Analyzes POST body before sending Reset (Reboot) request data to D-Bus.
     * OpenBMC supports ResetType "GracefulRestart" and "ForceRestart".
     */

    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/Actions/Manager.Reset/")
        .privileges(redfish::privileges::postManager)
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                BMCWEB_LOG_DEBUG << "Post Manager Reset.";

                std::string resetType;

                if (!json_util::readJson(req, asyncResp->res, "ResetType",
                                         resetType))
                {
                    return;
                }

                if (resetType == "GracefulRestart")
                {
                    BMCWEB_LOG_DEBUG << "Proceeding with " << resetType;
                    doBMCGracefulRestart(asyncResp);
                    return;
                }
                if (resetType == "ForceRestart")
                {
                    BMCWEB_LOG_DEBUG << "Proceeding with " << resetType;
                    doBMCForceRestart(asyncResp);
                    return;
                }
                BMCWEB_LOG_DEBUG << "Invalid property value for ResetType: "
                                 << resetType;
                messages::actionParameterNotSupported(asyncResp->res, resetType,
                                                      "ResetType");

                return;
            });
}

void requestRoutesManagerResetToDefaultsAction(App& app)
{

    /**
     * Function handles ResetToDefaults POST method request.
     *
     * Analyzes POST body message and factory resets BMC by calling
     * BMC code updater factory reset followed by a BMC reboot.
     *
     * BMC code updater factory reset wipes the whole BMC read-write
     * filesystem which includes things like the network settings.
     *
     * OpenBMC only supports ResetToDefaultsType "ResetAll".
     */

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Managers/bmc/Actions/Manager.ResetToDefaults/")
        .privileges(redfish::privileges::postManager)
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                BMCWEB_LOG_DEBUG << "Post ResetToDefaults.";

                std::string resetType;

                if (!json_util::readJson(req, asyncResp->res,
                                         "ResetToDefaultsType", resetType))
                {
                    BMCWEB_LOG_DEBUG << "Missing property ResetToDefaultsType.";

                    messages::actionParameterMissing(asyncResp->res,
                                                     "ResetToDefaults",
                                                     "ResetToDefaultsType");
                    return;
                }

                if (resetType != "ResetAll")
                {
                    BMCWEB_LOG_DEBUG << "Invalid property value for "
                                        "ResetToDefaultsType: "
                                     << resetType;
                    messages::actionParameterNotSupported(
                        asyncResp->res, resetType, "ResetToDefaultsType");
                    return;
                }

                crow::connections::systemBus->async_method_call(
                    [asyncResp](const boost::system::error_code ec) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG << "Failed to ResetToDefaults: "
                                             << ec;
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        // Factory Reset doesn't actually happen until a reboot
                        // Can't erase what the BMC is running on
                        doBMCGracefulRestart(asyncResp);
                    },
                    "xyz.openbmc_project.Software.BMC.Updater",
                    "/xyz/openbmc_project/software",
                    "xyz.openbmc_project.Common.FactoryReset", "Reset");
            });
}

void requestRoutesManagerResetActionInfo(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */

    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/ResetActionInfo/")
        .privileges(redfish::privileges::getActionInfo)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue = {
                    {"@odata.type", "#ActionInfo.v1_1_2.ActionInfo"},
                    {"@odata.id", "/redfish/v1/Managers/bmc/ResetActionInfo"},
                    {"Name", "Reset Action Info"},
                    {"Id", "ResetActionInfo"},
                    {"Parameters",
                     {{{"Name", "ResetType"},
                       {"Required", true},
                       {"DataType", "String"},
                       {"AllowableValues",
                        {"GracefulRestart", "ForceRestart"}}}}}};
            });
}

void requestRoutesManager(App& app)
{
    std::string uuid = persistent_data::getConfig().systemUuid;

    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/")
        .privileges(redfish::privileges::getManager)
        .methods(boost::beast::http::verb::get)([uuid](const crow::Request&,
                                                       const std::shared_ptr<
                                                           bmcweb::AsyncResp>&
                                                           asyncResp) {
            asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Managers/bmc";
            asyncResp->res.jsonValue["@odata.type"] =
                "#Manager.v1_11_0.Manager";
            asyncResp->res.jsonValue["Id"] = "bmc";
            asyncResp->res.jsonValue["Name"] = "OpenBmc Manager";
            asyncResp->res.jsonValue["Description"] =
                "Baseboard Management Controller";
            asyncResp->res.jsonValue["PowerState"] = "On";
            asyncResp->res.jsonValue["Status"] = {{"State", "Enabled"},
                                                  {"Health", "OK"}};
            asyncResp->res.jsonValue["ManagerType"] = "BMC";
            asyncResp->res.jsonValue["UUID"] = systemd_utils::getUuid();
            asyncResp->res.jsonValue["ServiceEntryPointUUID"] = uuid;
            asyncResp->res.jsonValue["Model"] =
                "OpenBmc"; // TODO(ed), get model

            asyncResp->res.jsonValue["LogServices"] = {
                {"@odata.id", "/redfish/v1/Managers/bmc/LogServices"}};

            asyncResp->res.jsonValue["NetworkProtocol"] = {
                {"@odata.id", "/redfish/v1/Managers/bmc/NetworkProtocol"}};

            asyncResp->res.jsonValue["EthernetInterfaces"] = {
                {"@odata.id", "/redfish/v1/Managers/bmc/EthernetInterfaces"}};

#ifdef BMCWEB_ENABLE_VM_NBDPROXY
            asyncResp->res.jsonValue["VirtualMedia"] = {
                {"@odata.id", "/redfish/v1/Managers/bmc/VirtualMedia"}};
#endif // BMCWEB_ENABLE_VM_NBDPROXY

            // default oem data
            nlohmann::json& oem = asyncResp->res.jsonValue["Oem"];
            nlohmann::json& oemOpenbmc = oem["OpenBmc"];
            oem["@odata.type"] = "#OemManager.Oem";
            oem["@odata.id"] = "/redfish/v1/Managers/bmc#/Oem";
            oemOpenbmc["@odata.type"] = "#OemManager.OpenBmc";
            oemOpenbmc["@odata.id"] = "/redfish/v1/Managers/bmc#/Oem/OpenBmc";
            oemOpenbmc["Certificates"] = {
                {"@odata.id",
                 "/redfish/v1/Managers/bmc/Truststore/Certificates"}};

            // Manager.Reset (an action) can be many values, OpenBMC only
            // supports BMC reboot.
            nlohmann::json& managerReset =
                asyncResp->res.jsonValue["Actions"]["#Manager.Reset"];
            managerReset["target"] =
                "/redfish/v1/Managers/bmc/Actions/Manager.Reset";
            managerReset["@Redfish.ActionInfo"] =
                "/redfish/v1/Managers/bmc/ResetActionInfo";

            // ResetToDefaults (Factory Reset) has values like
            // PreserveNetworkAndUsers and PreserveNetwork that aren't supported
            // on OpenBMC
            nlohmann::json& resetToDefaults =
                asyncResp->res.jsonValue["Actions"]["#Manager.ResetToDefaults"];
            resetToDefaults["target"] =
                "/redfish/v1/Managers/bmc/Actions/Manager.ResetToDefaults";
            resetToDefaults["ResetType@Redfish.AllowableValues"] = {"ResetAll"};

            std::pair<std::string, std::string> redfishDateTimeOffset =
                crow::utility::getDateTimeOffsetNow();

            asyncResp->res.jsonValue["DateTime"] = redfishDateTimeOffset.first;
            asyncResp->res.jsonValue["DateTimeLocalOffset"] =
                redfishDateTimeOffset.second;

            // TODO (Gunnar): Remove these one day since moved to ComputerSystem
            // Still used by OCP profiles
            // https://github.com/opencomputeproject/OCP-Profiles/issues/23
            // Fill in SerialConsole info
            asyncResp->res.jsonValue["SerialConsole"]["ServiceEnabled"] = true;
            asyncResp->res.jsonValue["SerialConsole"]["MaxConcurrentSessions"] =
                15;
            asyncResp->res.jsonValue["SerialConsole"]["ConnectTypesSupported"] =
                {"IPMI", "SSH"};
#ifdef BMCWEB_ENABLE_KVM
            // Fill in GraphicalConsole info
            asyncResp->res.jsonValue["GraphicalConsole"]["ServiceEnabled"] =
                true;
            asyncResp->res
                .jsonValue["GraphicalConsole"]["MaxConcurrentSessions"] = 4;
            asyncResp->res.jsonValue["GraphicalConsole"]
                                    ["ConnectTypesSupported"] = {"KVMIP"};
#endif // BMCWEB_ENABLE_KVM

            asyncResp->res.jsonValue["Links"]["ManagerForServers@odata.count"] =
                1;
            asyncResp->res.jsonValue["Links"]["ManagerForServers"] = {
                {{"@odata.id", "/redfish/v1/Systems/system"}}};

            auto health = std::make_shared<HealthPopulate>(asyncResp);
            health->isManagersHealth = true;
            health->populate();

            fw_util::populateFirmwareInformation(asyncResp, fw_util::bmcPurpose,
                                                 "FirmwareVersion", true);

            managerGetLastResetTime(asyncResp);

            auto pids = std::make_shared<GetPIDValues>(asyncResp);
            pids->run();

            getMainChassisId(
                asyncResp, [](const std::string& chassisId,
                              const std::shared_ptr<bmcweb::AsyncResp>& aRsp) {
                    aRsp->res
                        .jsonValue["Links"]["ManagerForChassis@odata.count"] =
                        1;
                    aRsp->res.jsonValue["Links"]["ManagerForChassis"] = {
                        {{"@odata.id", "/redfish/v1/Chassis/" + chassisId}}};
                    aRsp->res.jsonValue["Links"]["ManagerInChassis"] = {
                        {"@odata.id", "/redfish/v1/Chassis/" + chassisId}};
                });

            static bool started = false;

            if (!started)
            {
                crow::connections::systemBus->async_method_call(
                    [asyncResp](const boost::system::error_code ec,
                                const std::variant<double>& resp) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "Error while getting progress";
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        const double* val = std::get_if<double>(&resp);
                        if (val == nullptr)
                        {
                            BMCWEB_LOG_ERROR
                                << "Invalid response while getting progress";
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        if (*val < 1.0)
                        {
                            asyncResp->res.jsonValue["Status"]["State"] =
                                "Starting";
                            started = true;
                        }
                    },
                    "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
                    "org.freedesktop.DBus.Properties", "Get",
                    "org.freedesktop.systemd1.Manager", "Progress");
            }

            crow::connections::systemBus->async_method_call(
                [asyncResp](
                    const boost::system::error_code ec,
                    const std::vector<
                        std::pair<std::string,
                                  std::vector<std::pair<
                                      std::string, std::vector<std::string>>>>>&
                        subtree) {
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG
                            << "D-Bus response error on GetSubTree " << ec;
                        return;
                    }
                    if (subtree.size() == 0)
                    {
                        BMCWEB_LOG_DEBUG << "Can't find bmc D-Bus object!";
                        return;
                    }
                    // Assume only 1 bmc D-Bus object
                    // Throw an error if there is more than 1
                    if (subtree.size() > 1)
                    {
                        BMCWEB_LOG_DEBUG
                            << "Found more than 1 bmc D-Bus object!";
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    if (subtree[0].first.empty() ||
                        subtree[0].second.size() != 1)
                    {
                        BMCWEB_LOG_DEBUG << "Error getting bmc D-Bus object!";
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    const std::string& path = subtree[0].first;
                    const std::string& connectionName =
                        subtree[0].second[0].first;

                    for (const auto& interfaceName :
                         subtree[0].second[0].second)
                    {
                        if (interfaceName ==
                            "xyz.openbmc_project.Inventory.Decorator.Asset")
                        {
                            crow::connections::systemBus->async_method_call(
                                [asyncResp](
                                    const boost::system::error_code ec,
                                    const std::vector<
                                        std::pair<std::string,
                                                  std::variant<std::string>>>&
                                        propertiesList) {
                                    if (ec)
                                    {
                                        BMCWEB_LOG_DEBUG
                                            << "Can't get bmc asset!";
                                        return;
                                    }
                                    for (const std::pair<
                                             std::string,
                                             std::variant<std::string>>&
                                             property : propertiesList)
                                    {
                                        const std::string& propertyName =
                                            property.first;

                                        if ((propertyName == "PartNumber") ||
                                            (propertyName == "SerialNumber") ||
                                            (propertyName == "Manufacturer") ||
                                            (propertyName == "Model") ||
                                            (propertyName == "SparePartNumber"))
                                        {
                                            const std::string* value =
                                                std::get_if<std::string>(
                                                    &property.second);
                                            if (value == nullptr)
                                            {
                                                // illegal property
                                                messages::internalError(
                                                    asyncResp->res);
                                                return;
                                            }
                                            asyncResp->res
                                                .jsonValue[propertyName] =
                                                *value;
                                        }
                                    }
                                },
                                connectionName, path,
                                "org.freedesktop.DBus.Properties", "GetAll",
                                "xyz.openbmc_project.Inventory.Decorator."
                                "Asset");
                        }
                        else if (interfaceName ==
                                 "xyz.openbmc_project.Inventory."
                                 "Decorator.LocationCode")
                        {
                            getLocation(asyncResp, connectionName, path);
                        }
                    }
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                "/xyz/openbmc_project/inventory", int32_t(0),
                std::array<const char*, 1>{
                    "xyz.openbmc_project.Inventory.Item.Bmc"});
        });

    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/")
        .privileges(redfish::privileges::patchManager)
        .methods(
            boost::beast::http::verb::
                patch)([](const crow::Request& req,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
            std::optional<nlohmann::json> oem;
            std::optional<nlohmann::json> links;
            std::optional<std::string> datetime;

            if (!json_util::readJson(req, asyncResp->res, "Oem", oem,
                                     "DateTime", datetime, "Links", links))
            {
                return;
            }

            if (oem)
            {
                std::optional<nlohmann::json> openbmc;
                if (!redfish::json_util::readJson(*oem, asyncResp->res,
                                                  "OpenBmc", openbmc))
                {
                    BMCWEB_LOG_ERROR
                        << "Illegal Property "
                        << oem->dump(2, ' ', true,
                                     nlohmann::json::error_handler_t::replace);
                    return;
                }
                if (openbmc)
                {
                    std::optional<nlohmann::json> fan;
                    if (!redfish::json_util::readJson(*openbmc, asyncResp->res,
                                                      "Fan", fan))
                    {
                        BMCWEB_LOG_ERROR
                            << "Illegal Property "
                            << openbmc->dump(
                                   2, ' ', true,
                                   nlohmann::json::error_handler_t::replace);
                        return;
                    }
                    if (fan)
                    {
                        auto pid =
                            std::make_shared<SetPIDValues>(asyncResp, *fan);
                        pid->run();
                    }
                }
            }
            if (links)
            {
                std::optional<nlohmann::json> activeSoftwareImage;
                if (!redfish::json_util::readJson(*links, asyncResp->res,
                                                  "ActiveSoftwareImage",
                                                  activeSoftwareImage))
                {
                    return;
                }
                if (activeSoftwareImage)
                {
                    std::optional<std::string> odataId;
                    if (!json_util::readJson(*activeSoftwareImage,
                                             asyncResp->res, "@odata.id",
                                             odataId))
                    {
                        return;
                    }

                    if (odataId)
                    {
                        setActiveFirmwareImage(asyncResp, *odataId);
                    }
                }
            }
            if (datetime)
            {
                setDateTime(asyncResp, std::move(*datetime));
            }
        });
}

void requestRoutesManagerCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/")
        .privileges(redfish::privileges::getManagerCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                // Collections don't include the static data added by SubRoute
                // because it has a duplicate entry for members
                asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Managers";
                asyncResp->res.jsonValue["@odata.type"] =
                    "#ManagerCollection.ManagerCollection";
                asyncResp->res.jsonValue["Name"] = "Manager Collection";
                asyncResp->res.jsonValue["Members@odata.count"] = 1;
                asyncResp->res.jsonValue["Members"] = {
                    {{"@odata.id", "/redfish/v1/Managers/bmc"}}};
            });
}

}