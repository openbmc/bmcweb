#pragma once

#include <dbus_utility.hpp>
#include <human_sort.hpp>
#include <sdbusplus/asio/property.hpp>
#include <utility.hpp>
#include <utils/json_utils.hpp>
#include <utils/location_utils.hpp>

namespace redfish
{
inline void addAssemblyAssetProperties(
    const std::string& serviceName, const std::string& path,
    const std::shared_ptr<bmcweb::AsyncResp>& aResp, std::size_t index)
{
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, serviceName, path,
        "xyz.openbmc_project.Inventory.Decorator.Asset",
        [aResp, index](const boost::system::error_code ec,
                       const dbus::utility::DBusPropertiesMap& propertiesList) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "Failed to get asset properties: " << ec;
            messages::internalError(aResp->res);
            return;
        }

        nlohmann::json& assembliesJson = aResp->res.jsonValue["Assemblies"];
        if (!assembliesJson.is_array() || assembliesJson.size() <= index)
        {
            BMCWEB_LOG_ERROR << "Assemblies list too small";
            messages::internalError(aResp->res);
            return;
        }

        for (const auto& [propertyName, value] : propertiesList)
        {
            // This should really be a string_view, but nlohmann::json
            // doesn't support string_view keys.
            std::string jsonKey;
            if ((propertyName == "PartNumber") ||
                (propertyName == "SerialNumber") || (propertyName == "Model") ||
                (propertyName == "SparePartNumber"))
            {
                jsonKey = propertyName;
            }
            else if (propertyName == "Manufacturer")
            {
                jsonKey = "Producer";
            }
            else if (propertyName == "BuildDate")
            {
                jsonKey = "ProductionDate";
            }
            else
            {
                // Not supported.
                BMCWEB_LOG_WARNING << "Unsupported assembly asset "
                                   << propertyName;
                continue;
            }

            const auto* valueStr = std::get_if<std::string>(&value);
            if (valueStr == nullptr)
            {
                BMCWEB_LOG_ERROR << "Null value returned for " << propertyName;
                messages::internalError(aResp->res);
                return;
            }
            // SparePartNumber default is empty
            if (propertyName == "SparePartNumber" && valueStr->empty())
            {
                continue;
            }

            assembliesJson[index][jsonKey] = *valueStr;
        }
        });
}

/**
 * @brief Get properties for an assembly
 * @param[in] aResp - Shared pointer for asynchronous calls.
 * @param[in] assemblyId - Assembly resource ID.
 * @param[in] assemblies - list of all the dbus paths associated as an assembly
 * with the parent.
 * @return None.
 */
inline void
    getAssemblyProperties(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                          const boost::urls::url& assemblyId,
                          const std::vector<std::string>& assemblies)
{
    aResp->res.jsonValue["Assemblies@odata.count"] = assemblies.size();
    aResp->res.jsonValue["Assemblies"] = nlohmann::json::array();

    for (const std::string& assembly : assemblies)
    {
        std::string name = sdbusplus::message::object_path(assembly).filename();
        if (name.empty())
        {
            BMCWEB_LOG_DEBUG << "Empty name in assembly";
            messages::internalError(aResp->res);
            return;
        }

        crow::connections::systemBus->async_method_call(
            [aResp, assembly, assemblyId, name](
                const boost::system::error_code ec,
                const std::vector<
                    std::pair<std::string, std::vector<std::string>>>& object) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }

            nlohmann::json& assembliesJson = aResp->res.jsonValue["Assemblies"];
            std::size_t assemblyIndex = assembliesJson.size();
            boost::urls::url dataID = assemblyId;
            dataID.set_fragment(
                crow::utility::urlFromPieces(
                    "Assemblies", std::to_string(assembliesJson.size()))
                    .string());

            assembliesJson.push_back(
                {{"@odata.type", "#Assembly.v1_3_0.AssemblyData"},
                 {"@odata.id", dataID.string()},
                 {"MemberId", std::to_string(assemblyIndex)},
                 {"Name", name}});

            for (const auto& [serviceName, interfaceList] : object)
            {
                for (const auto& interface : interfaceList)
                {
                    if (interface ==
                        "xyz.openbmc_project.Inventory.Decorator.Asset")
                    {
                        addAssemblyAssetProperties(serviceName, assembly, aResp,
                                                   assemblyIndex);
                    }
                    else if (interface == "xyz.openbmc_project.Inventory."
                                          "Decorator.LocationCode")
                    {
                        location_util::getLocationCode(
                            aResp, serviceName, assembly,
                            "/Assemblies"_json_pointer / assemblyIndex /
                                "Location");
                    }
                    else
                    {
                        std::optional<std::string> type =
                            location_util::getLocationType(interface);
                        if (type)
                        {
                            assembliesJson[assemblyIndex]["Location"]
                                          ["PartLocation"]["LocationType"] =
                                              *type;
                        }
                    }
                }
            }
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetObject", assembly,
            std::to_array<std::string>(
                {"xyz.openbmc_project.Inventory.Decorator.Asset",
                 "xyz.openbmc_project.Inventory.Decorator.LocationCode",
                 "xyz.openbmc_project.Inventory.Connector.Embedded",
                 "xyz.openbmc_project.Inventory.Connector.Slot"}));
    }
}

/**
 * @brief Api to get assembly endpoints from mapper.
 * @param[in] aResp - Shared pointer for asynchronous calls.
 * @param[in] assemblyId - Assembly resource ID.
 * @param[in] assemblyPath - DBus path of assembly association.
 *
 * @return None.
 */
inline void
    getAssemblyEndpoints(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                         const boost::urls::url& assemblyId,
                         const std::string& assemblyPath)
{
    // if there is assembly association, look
    // for endpoints
    sdbusplus::asio::getProperty<std::vector<std::string>>(
        *crow::connections::systemBus, "xyz.openbmc_project.ObjectMapper",
        assemblyPath, "xyz.openbmc_project.Association", "endpoints",
        [aResp, assemblyId](const boost::system::error_code ec,
                            const std::vector<std::string>& endpoints) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error";
            messages::internalError(aResp->res);
            return;
        }

        std::vector<std::string> sortedAssemblyList = endpoints;
        std::sort(sortedAssemblyList.begin(), sortedAssemblyList.end(),
                  AlphanumLess<std::string>());

        getAssemblyProperties(aResp, assemblyId, sortedAssemblyList);
        });
}

/**
 * @brief Gets assemblies from assembly associations to an object.
 * @param[in] aResp - Shared pointer for asynchronous calls.
 * @param[in] assemblyId - Assembly resource ID.
 * @param[in] parentPath - DBus path of parent of assembly.
 *
 * @return None.
 */
inline void getAssembly(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                        const boost::urls::url& assemblyId,
                        const std::string& parentPath)
{
    aResp->res.jsonValue["@odata.type"] = "#Assembly.v1_3_0.Assembly";
    aResp->res.jsonValue["@odata.id"] = assemblyId.string();
    aResp->res.jsonValue["Name"] = "Assembly Collection";
    aResp->res.jsonValue["Id"] = "Assembly";

    aResp->res.jsonValue["Assemblies"] = nlohmann::json::array();
    aResp->res.jsonValue["Assemblies@odata.count"] = 0;

    // check if this chassis hosts any association
    crow::connections::systemBus->async_method_call(
        [aResp, assemblyId](const boost::system::error_code ec,
                            const std::vector<std::string>& paths) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error";
            messages::internalError(aResp->res);
            return;
        }
        for (const std::string& path : paths)
        {
            if (sdbusplus::message::object_path(path).filename() == "assembly")
            {
                getAssemblyEndpoints(aResp, assemblyId, path);
                return;
            }
        }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths", parentPath, 0,
        std::array<const char*, 1>{"xyz.openbmc_project.Association"});
}

} // namespace redfish
