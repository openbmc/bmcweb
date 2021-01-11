#pragma once

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
inline void processAssetProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp, std::size_t assemblyIndex,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& propertiesList)
{
    if (ec)
    {
        BMCWEB_LOG_DEBUG << "DBUS response error: " << ec.message();
        messages::internalError(aResp->res);
        return;
    }

    BMCWEB_LOG_DEBUG << "Got " << propertiesList.size()
                     << "properties for Asset";

    nlohmann::json& assemblyArray = aResp->res.jsonValue["Assemblies"];
    nlohmann::json& assemblyData = assemblyArray.at(assemblyIndex);

    const std::string* partNum = nullptr;
    const std::string* serialNum = nullptr;
    const std::string* sparePartNum = nullptr;
    const std::string* modelNum = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), propertiesList, "PartNumber", partNum,
        "SerialNumber", serialNum, "SparePartNumber", sparePartNum, "Model",
        modelNum);

    if (!success)
    {
        BMCWEB_LOG_ERROR << "Failed to get Asset properties";
        messages::internalError(aResp->res);
        return;
    }

    if (partNum == nullptr)
    {
        BMCWEB_LOG_ERROR << "Failed to get part number";
        messages::internalError(aResp->res);
        return;
    }
    assemblyData["PartNumber"] = *partNum;

    if (serialNum == nullptr)
    {
        BMCWEB_LOG_ERROR << "Failed to get serial number";
        messages::internalError(aResp->res);
        return;
    }
    assemblyData["SerialNumber"] = *serialNum;

    if (sparePartNum == nullptr)
    {
        BMCWEB_LOG_ERROR << "Failed to get spare part number";
        messages::internalError(aResp->res);
        return;
    }
    assemblyData["SparePartNumber"] = *sparePartNum;

    if (modelNum == nullptr)
    {
        BMCWEB_LOG_ERROR << "Failed to get model number";
        messages::internalError(aResp->res);
        return;
    }
    assemblyData["Model"] = *modelNum;
}

/**
 * @brief Get properties for the assemblies associated to given chassis
 * @param[in] aResp - Shared pointer for asynchronous calls.
 * @param[in] chassisPath - Chassis the assemblies are associated with.
 * @param[in] assemblies - list of all the assemblies associated with the
 * chassis.
 * @return None.
 */
inline void
    afterGetAssemblyProperties(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                               std::size_t assemblyIndex, const auto& assembly,
                               const boost::system::error_code& ec,
                               const ::dbus::utility::MapperGetObject& object)
{
    if (ec)
    {
        BMCWEB_LOG_DEBUG << "DBUS response error: " << ec.message();
        messages::internalError(aResp->res);
        return;
    }

    for (const auto& [serviceName, interfaceList] : object)
    {
        BMCWEB_LOG_DEBUG << "ServiceName: " << serviceName;
        for (const auto& interface : interfaceList)
        {
            BMCWEB_LOG_DEBUG << "interface: " << interface;
            if (interface == "xyz.openbmc_project.Inventory.Decorator.Asset")
            {
                sdbusplus::asio::getAllProperties(
                    *crow::connections::systemBus, serviceName, assembly,
                    "xyz.openbmc_project.Inventory.Decorator.Asset",
                    [aResp,
                     assemblyIndex](const boost::system::error_code& ec2,
                                    const dbus::utility::DBusPropertiesMap&
                                        propertiesList) {
                    processAssetProperties(aResp, assemblyIndex, ec2,
                                           propertiesList);
                    });
            }
            else if (interface == "xyz.openbmc_project.Inventory."
                                  "Decorator.LocationCode")
            {
                sdbusplus::asio::getProperty<std::string>(
                    *crow::connections::systemBus, serviceName, assembly,
                    "xyz.openbmc_project.Inventory.Decorator.LocationCode",
                    "LocationCode",
                    [aResp, assemblyIndex](const boost::system::error_code& ec3,
                                           const std::string& location) {
                    if (ec3)
                    {
                        BMCWEB_LOG_DEBUG << "DBUS response error: "
                                         << ec3.message();
                        messages::internalError(aResp->res);
                        return;
                    }

                    nlohmann::json& assemblyArray =
                        aResp->res.jsonValue["Assemblies"];
                    nlohmann::json& assemblyData =
                        assemblyArray.at(assemblyIndex);

                    assemblyData["Location"]["PartLocation"]["ServiceLabel"] =
                        location;
                    });
            }
        }
    }
}

/**
 * @brief Get properties for the assemblies associated to given chassis
 * @param[in] aResp - Shared pointer for asynchronous calls.
 * @param[in] chassisPath - Chassis the assemblies are associated with.
 * @param[in] assemblies - list of all the assemblies associated with
 * the chassis.
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

        std::string dataID = "/redfish/v1/Chassis/" + chassis +
                             "/Assembly#/Assemblies/";
        dataID.append(std::to_string(assemblyIndex));

        tempyArray.push_back({{"@odata.type", "#Assembly.v1_3_0.AssemblyData"},
                              {"@odata.id", dataID},
                              {"MemberId", std::to_string(assemblyIndex)}});

        tempyArray.at(assemblyIndex)["Name"] =
            sdbusplus::message::object_path(assembly).filename();

        constexpr std::array<std::string_view, 5> interfaces = {
            "xyz.openbmc_project.Inventory.Item.Vrm",
            "xyz.openbmc_project.Inventory.Item.Tpm",
            "xyz.openbmc_project.Inventory.Item.Panel",
            "xyz.openbmc_project.Inventory.Item.Battery",
            "xyz.openbmc_project.Inventory.Item.DiskBackplane"};

        dbus::utility::getDbusObject(
            assembly, interfaces,
            [aResp, assemblyIndex,
             assembly](const boost::system::error_code& ec,
                       const ::dbus::utility::MapperGetObject& objInfo) {
            afterGetAssemblyProperties(aResp, assemblyIndex, assembly, ec,
                                       objInfo);
            });

        assemblyIndex++;
    }
}

/**
 * @brief Api to check if the assemblies fetched from association Json
 * is also implemented in the system. In case the interface for that
 * assembly is not found update the list and fetch properties for only
 * implemented assemblies.
 * @param[in] aResp - Shared pointer for asynchronous calls.
 * @param[in] chassisPath - Chassis the assemblies are associated with.
 * @param[in] assemblies - list of all the assemblies associated with
 * the chassis.
 * @return None.
 */
inline void afterAssemblyInterface(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const std::string& chassisPath, const std::vector<std::string>& assemblies,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        BMCWEB_LOG_DEBUG << "D-Bus response error on GetSubTree "
                         << ec.message();
        messages::internalError(aResp->res);
        return;
    }

    if (subtree.empty())
    {
        BMCWEB_LOG_DEBUG << "No object paths found";
        return;
    }
    std::vector<std::string> updatedAssemblyList;
    for (const auto& [objectPath, serviceName] : subtree)
    {
        BMCWEB_LOG_DEBUG << "ObjectPath: " << objectPath;

        // This list will store common paths between assemblies fetched
        // from association json and assemblies which are actually
        // implemeted. This is to handle the case in which there is
        // entry in association json but implementation of interface for
        // that particular assembly is missing.
        auto it = std::find(assemblies.begin(), assemblies.end(), objectPath);
        if (it != assemblies.end())
        {
            updatedAssemblyList.emplace(updatedAssemblyList.end(), *it);
        }
    }

    if (!updatedAssemblyList.empty())
    {
        // sorting is required to facilitate patch as the array does not
        // store and data which can be mapped back to Dbus path of
        // assembly.
        std::sort(updatedAssemblyList.begin(), updatedAssemblyList.end());

        getAssemblyProperties(aResp, chassisPath, updatedAssemblyList);
    }
}

/**
 * @brief Api to check if the assemblies fetched from association Json
 * is also implemented in the system. In case the interface for that
 * assembly is not found update the list and fetch properties for only
 * implemented assemblies.
 * @param[in] aResp - Shared pointer for asynchronous calls.
 * @param[in] chassisPath - Chassis the assemblies are associated with.
 * @param[in] assemblies - list of all the assemblies associated with
 * the chassis.
 * @return None.
 */
inline void
    checkAssemblyInterface(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                           const std::string& chassisPath,
                           std::vector<std::string>& assemblies)
{
    constexpr std::array<std::string_view, 5> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Vrm",
        "xyz.openbmc_project.Inventory.Item.Tpm",
        "xyz.openbmc_project.Inventory.Item.Panel",
        "xyz.openbmc_project.Inventory.Item.Battery",
        "xyz.openbmc_project.Inventory.Item.DiskBackplane"};

    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [aResp, chassisPath,
         assemblies](const boost::system::error_code& ec,
                     const dbus::utility::MapperGetSubTreeResponse& subtree) {
        afterAssemblyInterface(aResp, chassisPath, assemblies, ec, subtree);
        });
}

/**
 * @brief Api to get assembly endpoints from mapper.
 * @param[in] aResp - Shared pointer for asynchronous calls.
 * @param[in] chassisPath - Chassis to which the assemblies are
 * associated.
 *
 * @return None.
 */
inline void
    getAssemblyEndpoints(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                         const std::string& chassisPath)
{
    BMCWEB_LOG_DEBUG << "Get assembly endpoints";

    sdbusplus::message::object_path assemblyPath(chassisPath);
    assemblyPath /= "assembly";

    // if there is assembly association, look
    // for endpoints
    crow::connections::systemBus->async_method_call(
        [aResp,
         chassisPath](const boost::system::error_code ec,
                      const std::variant<std::vector<std::string>>& endpoints) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error: " << ec.message();
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

        checkAssemblyInterface(aResp, chassisPath, sortedAssemblyList);
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
 *
 * @return None.
 */
inline void checkForAssemblyAssociations(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const std::string& chassisPath, const std::string& service)
{
    BMCWEB_LOG_DEBUG << "Check for assembly association";

    using associationList =
        std::vector<std::tuple<std::string, std::string, std::string>>;

    crow::connections::systemBus->async_method_call(
        [aResp,
         chassisPath](const boost::system::error_code ec,
                      const std::variant<associationList>& associations) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error: " << ec.message();
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
            BMCWEB_LOG_DEBUG << "Associations: "
                             << std::get<0>(listOfAssociations);
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
            getAssemblyEndpoints(aResp, chassisPath);
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
 *
 * @return None.
 */
inline void checkAssociation(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                             const std::string& chassisPath)
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

    // mapper call lambda
    constexpr std::array<std::string_view, 0> interfaces = {};

    dbus::utility::getDbusObject(
        chassisPath, interfaces,
        [aResp, chassisPath](const boost::system::error_code& ec,
                             const ::dbus::utility::MapperGetObject& object) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error: " << ec.message();
            messages::internalError(aResp->res);
            return;
        }

        for (const auto& [serviceName, interfaceList] : object)
        {
            BMCWEB_LOG_DEBUG << "serviceName: " << serviceName;
            for (const auto& interface : interfaceList)
            {
                BMCWEB_LOG_DEBUG << "Interface: " << interface;
                if (interface == "xyz.openbmc_project.Association.Definitions")
                {
                    checkForAssemblyAssociations(aResp, chassisPath,
                                                 serviceName);
                    return;
                }
            }
        }
        return;
        });
}

namespace assembly
{
/**
 * @brief Get chassis path with given chassis ID
 * @param[in] aResp - Shared pointer for asynchronous calls.
 * @param[in] chassisID - Chassis to which the assemblies are
 * associated.
 *
 * @return None.
 */
inline void processChasisPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const std::string& chassisID, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& chassisPaths)
{
    if (ec)
    {
        BMCWEB_LOG_DEBUG << "DBUS response error: " << ec.message();
        messages::internalError(aResp->res);
        return;
    }

    // check if the chassis path belongs to the chassis ID passed
    for (const auto& path : chassisPaths)
    {
        BMCWEB_LOG_DEBUG << "Chassis Path from Mapper " << path;
        std::string chassis = sdbusplus::message::object_path(path).filename();
        if (chassis != chassisID)
        {
            // this is not the chassis we are interested in
            continue;
        }

        aResp->res.jsonValue["@odata.type"] = "#Assembly.v1_3_0.Assembly";
        aResp->res.jsonValue["@odata.id"] = "/redfish/v1/Chassis/" + chassisID +
                                            "/Assembly";
        aResp->res.jsonValue["Name"] = "Assembly Collection";
        aResp->res.jsonValue["Id"] = "Assembly";

        checkAssociation(aResp, path);
        return;
    }

    BMCWEB_LOG_ERROR << "Chassis not found";
    messages::resourceNotFound(aResp->res, "Chassis", chassisID);
}

/**
 * @brief Get chassis path with given chassis ID
 * @param[in] aResp - Shared pointer for asynchronous calls.
 * @param[in] chassisID - Chassis to which the assemblies are
 * associated.
 *
 * @return None.
 */
inline void getChassis(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                       const std::string& chassisID)
{
    BMCWEB_LOG_DEBUG << "Get chassis path";

    constexpr std::array<std::string_view, 2> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Chassis",
        "xyz.openbmc_project.Inventory.Item.Board"};

    // Use mapper to get chassis paths.
    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/inventory", 0, interfaces,
        [aResp, chassisID](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& chassisPaths) {
        processChasisPaths(aResp, chassisID, ec, chassisPaths);
        });
}
} // namespace assembly

inline void
    handleAssemplyHead(App& app, const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& /* chassisId */)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/Assembly/Assembly.json>; rel=describedby");
}

/**
 * Systems derived class for delivering Assembly Schema.
 */
inline void requestRoutesAssembly(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Assembly/")
        .privileges(redfish::privileges::privilegeSetLogin)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleAssemplyHead, std::ref(app)));

    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Assembly/")
        .privileges(redfish::privileges::privilegeSetLogin)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& chassisId) {
        BMCWEB_LOG_DEBUG << "chassis =" << chassisId;
        assembly::getChassis(asyncResp, chassisId);
        });
}
} // namespace redfish
