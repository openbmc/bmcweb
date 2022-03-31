#pragma once

#include <utils/asset_utils.hpp>
#include <utils/json_utils.hpp>
#include <utils/location_utils.hpp>

namespace redfish
{
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
                          const std::string& assemblyId,
                          const std::vector<std::string>& assemblies)
{
    aResp->res.jsonValue["Assemblies@odata.count"] = assemblies.size();
    nlohmann::json& assembliesJson = aResp->res.jsonValue["Assemblies"];

    for (const std::string& assembly : assemblies)
    {
        std::size_t assemblyIndex = assembliesJson.size();
        std::string dataID = assemblyId;
        // Replace trailing '/' with '#'
        dataID[dataID.length() - 1] = '#';
        dataID.append("/Assemblies/");
        dataID.append(std::to_string(assemblyIndex));

        assembliesJson.push_back(
            {{"@odata.type", "#Assembly.v1_3_0.AssemblyData"},
             {"@odata.id", dataID},
             {"MemberId", std::to_string(assemblyIndex)}});

        assembliesJson.at(assemblyIndex)["Name"] =
            sdbusplus::message::object_path(assembly).filename();

        crow::connections::systemBus->async_method_call(
            [aResp, assemblyIndex, assembly, &assembliesJson](
                const boost::system::error_code ec,
                const std::vector<
                    std::pair<std::string, std::vector<std::string>>>& object) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG << "DBUS response error";
                    messages::internalError(aResp->res);
                    return;
                }

                for (const auto& [serviceName, interfaceList] : object)
                {
                    for (const auto& interface : interfaceList)
                    {
                        if (interface ==
                            "xyz.openbmc_project.Inventory.Decorator.Asset")
                        {
                            asset_utils::addAssetProperties(
                                aResp, serviceName, assembly,
                                "/Assemblies"_json_pointer / assemblyIndex,
                                [](const std::string& name) {
                                    return name == "PartNumber" ||
                                           name == "SerialNumber" ||
                                           name == "SparePartNumber" ||
                                           name == "Model";
                                });
                        }
                        else if (interface == "xyz.openbmc_project.Inventory."
                                              "Decorator.LocationCode")
                        {
                            location_util::getLocationCode(
                                aResp, serviceName, assembly,
                                "/Assemblies"_json_pointer / assemblyIndex);
                        }
                        else
                        {
                            std::optional<std::string> type =
                                location_util::getLocationType(interface);
                            if (type.has_value())
                            {
                                assembliesJson[assemblyIndex]["PartLocation"]
                                              ["Type"] = *type;
                            }
                        }
                    }
                }
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetObject", assembly,
            std::array<const char*, 0>());
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
                         const std::string& assemblyId,
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
            std::sort(sortedAssemblyList.begin(), sortedAssemblyList.end());

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
                        const std::string& assemblyId,
                        const std::string& parentPath)
{
    aResp->res.jsonValue["@odata.type"] = "#Assembly.v1_3_0.Assembly";
    aResp->res.jsonValue["@odata.id"] = assemblyId;
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
                if (sdbusplus::message::object_path(path).filename() ==
                    "assembly")
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
