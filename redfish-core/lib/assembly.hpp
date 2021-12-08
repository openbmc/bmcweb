#pragma once

#include "led.hpp"

#include <utils/json_utils.hpp>
#include <utils/name_utils.hpp>

#include <variant>

namespace redfish
{
using VariantType = std::variant<bool, std::string, uint64_t, uint32_t>;

/**
 * @brief Get properties for the assemblies associated to given chassis
 * @param[in] aResp - Shared pointer for asynchronous calls.
 * @param[in] chassisPath - Chassis the assemblies are associated with.
 * @param[in] assemblies - list of all the assemblies associated with the
 * chassis.
 * @return None.
 */
inline void
    getAssemblyProperties(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                          const std::string& chassisPath,
                          const std::vector<std::string>& assemblies)
{
    BMCWEB_LOG_DEBUG << "Get properties for assembly associated";

    const std::string& chassis =
        sdbusplus::message::object_path(chassisPath).filename();

    aResp->res.jsonValue["Assemblies@odata.count"] = assemblies.size();

    std::size_t assemblyIndex = 0;
    for (const auto& assembly : assemblies)
    {
        nlohmann::json& tempyArray = aResp->res.jsonValue["Assemblies"];

        std::string dataID =
            "/redfish/v1/Chassis/" + chassis + "/Assembly#/Assemblies/";
        dataID.append(std::to_string(assemblyIndex));

        tempyArray.push_back({{"@odata.type", "#Assembly.v1_3_0.AssemblyData"},
                              {"@odata.id", dataID},
                              {"MemberId", std::to_string(assemblyIndex)}});

        tempyArray.at(assemblyIndex)["Name"] =
            sdbusplus::message::object_path(assembly).filename();

        crow::connections::systemBus->async_method_call(
            [aResp, assemblyIndex, assembly](
                const boost::system::error_code ec,
                const std::vector<
                    std::pair<std::string, std::vector<std::string>>>& object) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG << "DBUS response error";
                    messages::internalError(aResp->res);
                    return;
                }

                nlohmann::json_pointer<nlohmann::json> ptr(
                    "/Assemblies/" + std::to_string(assemblyIndex) + "/Name");

                name_util::getPrettyName(aResp, assembly, object, ptr);

                for (const auto& [serviceName, interfaceList] : object)
                {
                    for (const auto& interface : interfaceList)
                    {
                        if (interface ==
                            "xyz.openbmc_project.Inventory.Decorator.Asset")
                        {
                            crow::connections::systemBus->async_method_call(
                                [aResp, assemblyIndex](
                                    const boost::system::error_code ec2,
                                    const std::vector<
                                        std::pair<std::string, VariantType>>&
                                        propertiesList) {
                                    if (ec2)
                                    {
                                        BMCWEB_LOG_DEBUG
                                            << "DBUS response error";
                                        messages::internalError(aResp->res);
                                        return;
                                    }

                                    nlohmann::json& assemblyArray =
                                        aResp->res.jsonValue["Assemblies"];
                                    nlohmann::json& assemblyData =
                                        assemblyArray.at(assemblyIndex);

                                    for (const std::pair<std::string,
                                                         VariantType>&
                                             property : propertiesList)
                                    {
                                        if (property.first == "PartNumber")
                                        {
                                            const std::string* value =
                                                std::get_if<std::string>(
                                                    &property.second);
                                            if (value == nullptr)
                                            {
                                                messages::internalError(
                                                    aResp->res);
                                                return;
                                            }
                                            assemblyData["PartNumber"] = *value;
                                        }
                                        else if (property.first ==
                                                 "SerialNumber")
                                        {
                                            const std::string* value =
                                                std::get_if<std::string>(
                                                    &property.second);
                                            if (value == nullptr)
                                            {
                                                messages::internalError(
                                                    aResp->res);
                                                return;
                                            }
                                            assemblyData["SerialNumber"] =
                                                *value;
                                        }
                                        else if (property.first ==
                                                 "SparePartNumber")
                                        {
                                            const std::string* value =
                                                std::get_if<std::string>(
                                                    &property.second);
                                            if (value == nullptr)
                                            {
                                                messages::internalError(
                                                    aResp->res);
                                                return;
                                            }
                                            assemblyData["SparePartNumber"] =
                                                *value;
                                        }
                                        else if (property.first == "Model")
                                        {
                                            const std::string* value =
                                                std::get_if<std::string>(
                                                    &property.second);
                                            if (value == nullptr)
                                            {
                                                messages::internalError(
                                                    aResp->res);
                                                return;
                                            }
                                            assemblyData["Model"] = *value;
                                        }
                                    }
                                },
                                serviceName, assembly,
                                "org.freedesktop.DBus.Properties", "GetAll",
                                "xyz.openbmc_project.Inventory.Decorator."
                                "Asset");
                        }
                        else if (interface == "xyz.openbmc_project.Inventory."
                                              "Decorator.LocationCode")
                        {
                            crow::connections::systemBus->async_method_call(
                                [aResp, assemblyIndex](
                                    const boost::system::error_code ec,
                                    const std::variant<std::string>& property) {
                                    if (ec)
                                    {
                                        BMCWEB_LOG_DEBUG
                                            << "DBUS response error";
                                        messages::internalError(aResp->res);
                                        return;
                                    }

                                    nlohmann::json& assemblyArray =
                                        aResp->res.jsonValue["Assemblies"];
                                    nlohmann::json& assemblyData =
                                        assemblyArray.at(assemblyIndex);

                                    const std::string* value =
                                        std::get_if<std::string>(&property);

                                    if (value == nullptr)
                                    {
                                        // illegal value
                                        messages::internalError(aResp->res);
                                        return;
                                    }
                                    assemblyData["Location"]["PartLocation"]
                                                ["ServiceLabel"] = *value;
                                },
                                serviceName, assembly,
                                "org.freedesktop.DBus.Properties", "Get",
                                "xyz.openbmc_project.Inventory.Decorator."
                                "LocationCode",
                                "LocationCode");
                        }
                    }
                }

                nlohmann::json& assemblyArray =
                    aResp->res.jsonValue["Assemblies"];
                nlohmann::json& assemblyData = assemblyArray.at(assemblyIndex);
                getLocationIndicatorActive(aResp, assembly, assemblyData);
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetObject", assembly,
            std::array<const char*, 9>{
                "xyz.openbmc_project.Inventory.Item.Vrm",
                "xyz.openbmc_project.Inventory.Item.Tpm",
                "xyz.openbmc_project.Inventory.Item.Panel",
                "xyz.openbmc_project.Inventory.Item.Battery",
                "xyz.openbmc_project.Inventory.Item.DiskBackplane",
                "xyz.openbmc_project.Inventory.Item.Board",
                "xyz.openbmc_project.Inventory.Item.Connector",
                "xyz.openbmc_project.Inventory.Item.Drive",
                "xyz.openbmc_project.Inventory.Item.Board.Motherboard"});

        assemblyIndex++;
    }
}

/**
 * @brief Set location indicator for the assemblies associated to given chassis
 * @param[in] aResp - Shared pointer for asynchronous calls.
 * @param[in] chassis - Chassis the assemblies are associated with.
 * @param[in] assemblies - list of all the assemblies associated with the
 * chassis.
 * @param[in] req - The request data
 * @return None.
 */
inline void setAssemblylocationIndicators(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassis, const std::vector<std::string>& assemblies,
    const crow::Request& req)
{
    BMCWEB_LOG_DEBUG
        << "Set locationIndicator for assembly associated to chassis ="
        << chassis;

    std::optional<std::vector<nlohmann::json>> assemblyData;
    if (!json_util::readJson(req, asyncResp->res, "Assemblies", assemblyData))
    {
        return;
    }
    if (!assemblyData)
    {
        return;
    }

    std::vector<nlohmann::json> items = std::move(*assemblyData);
    std::map<std::string, bool> locationIndicatorActiveMap;

    for (auto& item : items)
    {
        bool locationIndicatorActive;
        std::string memberId;

        if (!json_util::readJson(item, asyncResp->res,
                                 "LocationIndicatorActive",
                                 locationIndicatorActive, "MemberId", memberId))
        {
            return;
        }
        locationIndicatorActiveMap[memberId] = locationIndicatorActive;
    }

    std::size_t assemblyIndex = 0;
    for (const auto& assembly : assemblies)
    {
        auto iter =
            locationIndicatorActiveMap.find(std::to_string(assemblyIndex));

        if (iter != locationIndicatorActiveMap.end())
        {
            setLocationIndicatorActive(asyncResp, assembly, iter->second);
        }
        assemblyIndex++;
    }

    return;
}

/**
 * @brief Api to check if the assemblies fetched from association Json is also
 * implemented in the system. In case the interface for that assembly is not
 * found update the list and fetch properties for only implemented assemblies.
 * @param[in] aResp - Shared pointer for asynchronous calls.
 * @param[in] chassisPath - Chassis the assemblies are associated with.
 * @param[in] assemblies - list of all the assemblies associated with the
 * chassis.
 * @param[in] setLocationIndicatorActiveFlag - The doPatch flag.
 * @param[in] req - The request data.
 * @return None.
 */
inline void checkAssemblyInterface(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const std::string& chassisPath, std::vector<std::string>& assemblies,
    const bool& setLocationIndicatorActiveFlag, const crow::Request& req)
{
    crow::connections::systemBus->async_method_call(
        [aResp, chassisPath, assemblies, setLocationIndicatorActiveFlag, req](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>&
                subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "D-Bus response error on GetSubTree " << ec;
                messages::internalError(aResp->res);
                return;
            }

            if (subtree.size() == 0)
            {
                BMCWEB_LOG_DEBUG << "No object paths found";
                return;
            }
            std::vector<std::string> updatedAssemblyList;
            for (const auto& [objectPath, serviceName] : subtree)
            {
                // This list will store common paths between assemblies fetched
                // from association json and assemblies which are actually
                // implemeted. This is to handle the case in which there is
                // entry in association json but implementation of interface for
                // that particular assembly is missing.
                auto it =
                    std::find(assemblies.begin(), assemblies.end(), objectPath);
                if (it != assemblies.end())
                {
                    updatedAssemblyList.emplace(updatedAssemblyList.end(), *it);
                }
            }

            if (updatedAssemblyList.size() != 0)
            {
                // sorting is required to facilitate patch as the array does not
                // store and data which can be mapped back to Dbus path of
                // assembly.
                std::sort(updatedAssemblyList.begin(),
                          updatedAssemblyList.end());

                if (setLocationIndicatorActiveFlag)
                {
                    setAssemblylocationIndicators(aResp, chassisPath,
                                                  updatedAssemblyList, req);
                }
                else
                {
                    getAssemblyProperties(aResp, chassisPath,
                                          updatedAssemblyList);
                }
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", int32_t(0),
        std::array<const char*, 9>{
            "xyz.openbmc_project.Inventory.Item.Vrm",
            "xyz.openbmc_project.Inventory.Item.Tpm",
            "xyz.openbmc_project.Inventory.Item.Panel",
            "xyz.openbmc_project.Inventory.Item.Battery",
            "xyz.openbmc_project.Inventory.Item.DiskBackplane",
            "xyz.openbmc_project.Inventory.Item.Board",
            "xyz.openbmc_project.Inventory.Item.Connector",
            "xyz.openbmc_project.Inventory.Item.Drive",
            "xyz.openbmc_project.Inventory.Item.Board.Motherboard"});
}

/**
 * @brief Api to get assembly endpoints from mapper.
 * @param[in] aResp - Shared pointer for asynchronous calls.
 * @param[in] chassisPath - Chassis to which the assemblies are
 * associated.
 * @param[in] setLocationIndicatorActiveFlag - The doPatch flag.
 * @param[in] req - The request data.
 * @return None.
 */
inline void
    getAssemblyEndpoints(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                         const std::string& chassisPath,
                         const bool& setLocationIndicatorActiveFlag,
                         const crow::Request& req)
{

    BMCWEB_LOG_DEBUG << "Get assembly endpoints";

    sdbusplus::message::object_path assemblyPath(chassisPath);
    assemblyPath /= "assembly";

    // if there is assembly association, look
    // for endpoints
    crow::connections::systemBus->async_method_call(
        [aResp, chassisPath, setLocationIndicatorActiveFlag,
         req](const boost::system::error_code ec,
              const std::variant<std::vector<std::string>>& endpoints) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response "
                                    "error";
                messages::internalError(aResp->res);
                return;
            }

            const std::vector<std::string>* assemblyList =
                std::get_if<std::vector<std::string>>(&(endpoints));

            if (assemblyList == nullptr)
            {
                BMCWEB_LOG_DEBUG << "No assembly found";
                return;
            }

            std::vector<std::string> sortedAssemblyList = *assemblyList;
            std::sort(sortedAssemblyList.begin(), sortedAssemblyList.end());

            checkAssemblyInterface(aResp, chassisPath, sortedAssemblyList,
                                   setLocationIndicatorActiveFlag, req);
            return;
        },
        "xyz.openbmc_project.ObjectMapper", assemblyPath,
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Association", "endpoints");
}

/**
 * @brief Api to check for assembly associations.
 * @param[in] aResp - Shared pointer for asynchronous calls.
 * @param[in] chassisPath - Chassis to which the assemblies are
 * associated.
 * @param[in] setLocationIndicatorActiveFlag - The doPatch flag.
 * @param[in] req - The request data.
 * @return None.
 */
inline void checkForAssemblyAssociations(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const std::string& chassisPath, const std::string& service,
    const bool& setLocationIndicatorActiveFlag, const crow::Request& req)
{
    BMCWEB_LOG_DEBUG << "Check for assembly association";

    using associationList =
        std::vector<std::tuple<std::string, std::string, std::string>>;

    crow::connections::systemBus->async_method_call(
        [aResp, chassisPath, setLocationIndicatorActiveFlag,
         req](const boost::system::error_code ec,
              const std::variant<associationList>& associations) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }

            const associationList* value =
                std::get_if<associationList>(&associations);
            if (value == nullptr)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }

            bool isAssmeblyAssociation = false;
            for (const auto& listOfAssociations : *value)
            {
                if (std::get<0>(listOfAssociations) != "assembly")
                {
                    // implies this is not an assembly
                    // association
                    continue;
                }

                isAssmeblyAssociation = true;
                break;
            }

            if (isAssmeblyAssociation)
            {
                getAssemblyEndpoints(aResp, chassisPath,
                                     setLocationIndicatorActiveFlag, req);
            }
        },
        service, chassisPath, "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Association.Definitions", "Associations");
}

/**
 * @brief Api to check if there is any association.
 * @param[in] aResp - Shared pointer for asynchronous calls.
 * @param[in] chassisPath - Chassis to which the assemblies are
 * associated.
 * @param[in] setLocationIndicatorActiveFlag - The doPatch flag.
 * @param[in] req - The request data.
 * @return None.
 */
inline void checkAssociation(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                             const std::string& chassisPath,
                             const bool& setLocationIndicatorActiveFlag,
                             const crow::Request& req)
{
    BMCWEB_LOG_DEBUG << "Check chassis for association";

    std::string chassis =
        sdbusplus::message::object_path(chassisPath).filename();
    if (chassis.empty())
    {
        BMCWEB_LOG_ERROR << "Failed to find / in Chassis path";
        messages::internalError(aResp->res);
        return;
    }

    aResp->res.jsonValue["Assemblies"] = nlohmann::json::array();
    aResp->res.jsonValue["Assemblies@odata.count"] = 0;

    // check if this chassis hosts any association
    crow::connections::systemBus->async_method_call(
        [aResp, chassisPath, setLocationIndicatorActiveFlag, req](
            const boost::system::error_code ec,
            const std::vector<std::pair<std::string, std::vector<std::string>>>&
                object) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }

            for (const auto& [serviceName, interfaceList] : object)
            {
                for (auto& interface : interfaceList)
                {
                    if (interface ==
                        "xyz.openbmc_project.Association.Definitions")
                    {
                        checkForAssemblyAssociations(
                            aResp, chassisPath, serviceName,
                            setLocationIndicatorActiveFlag, req);
                        return;
                    }
                }
            }
            return;
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", chassisPath,
        std::array<const char*, 0>{});
}

namespace assembly
{
/**
 * @brief Get chassis path with given chassis ID
 * @param[in] aResp - Shared pointer for asynchronous calls.
 * @param[in] chassisID - Chassis to which the assemblies are
 * associated.
 * @param[in] setLocationIndicatorActiveFlag - The doPatch flag.
 * @param[in] req - The request data.
 * @return None.
 */
inline void getChassis(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                       const std::string& chassisID,
                       const bool& setLocationIndicatorActiveFlag,
                       const crow::Request& req)
{
    BMCWEB_LOG_DEBUG << "Get chassis path";

    // get the chassis path
    crow::connections::systemBus->async_method_call(
        [aResp, chassisID, setLocationIndicatorActiveFlag,
         req](const boost::system::error_code ec,
              const std::vector<std::string>& chassisPaths) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }

            // check if the chassis path belongs to the chassis ID passed
            for (const auto& path : chassisPaths)
            {
                BMCWEB_LOG_DEBUG << "Chassis Paths from Mapper " << path;
                std::string chassis =
                    sdbusplus::message::object_path(path).filename();
                if (chassis != chassisID)
                {
                    // this is not the chassis we are interested in
                    continue;
                }

                aResp->res.jsonValue["@odata.type"] =
                    "#Assembly.v1_3_0.Assembly";
                aResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Chassis/" + chassisID + "/Assembly";
                aResp->res.jsonValue["Name"] = "Assembly Collection";
                aResp->res.jsonValue["Id"] = "Assembly";

                checkAssociation(aResp, path, setLocationIndicatorActiveFlag,
                                 req);
                return;
            }

            BMCWEB_LOG_ERROR << "Chassis not found";
            messages::resourceNotFound(aResp->res, "Chassis", chassisID);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        "/xyz/openbmc_project/inventory", 0,
        std::array<const char*, 1>{
            "xyz.openbmc_project.Inventory.Item.Chassis"});
}
} // namespace assembly

/**
 * Systems derived class for delivering Assembly Schema.
 */
inline void requestRoutesAssembly(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Assembly/")
        .privileges({{"Login"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& chassisId) {
                const bool setLocationIndicatorActiveFlag = false;

                BMCWEB_LOG_DEBUG << "chassis =" << chassisId;
                assembly::getChassis(asyncResp, chassisId,
                                     setLocationIndicatorActiveFlag, req);
            });

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Assembly/")
        .privileges({{"ConfigureComponents"}})
        .methods(boost::beast::http::verb::patch)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& chassisID) {
                const bool setLocationIndicatorActiveFlag = true;

                BMCWEB_LOG_DEBUG << "Chassis = " << chassisID;
                assembly::getChassis(asyncResp, chassisID,
                                     setLocationIndicatorActiveFlag, req);
            });
}
} // namespace redfish
