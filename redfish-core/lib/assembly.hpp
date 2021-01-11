#pragma once

#include <node.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{
/**
 * @brief Get properties for the assemblies associated to given chassis
 * @param[in] aResp - Shared pointer for asynchronous calls.
 * @param[in] assemblies - list of all the assemblies associated with the
 * chassis.
 * @param[in] chassisID - Chassis to which the assemblies are
 * associated.
 *
 * @return None.
 */
inline void getAssemblyProperties(const std::shared_ptr<AsyncResp>& aResp,
                                  const std::vector<std::string>& assemblies,
                                  const std::string& chassisID)
{
    BMCWEB_LOG_DEBUG << "Get properties for assembly associated";

    aResp->res.jsonValue["Assemblies"] = nlohmann::json::array();
    aResp->res.jsonValue["Assemblies@odata.count"] = assemblies.size();

    for (const auto& assembly : assemblies)
    {
        crow::connections::systemBus->async_method_call(
            [aResp, chassisID(std::string(chassisID)),
             assembly(std::string(assembly))](
                const boost::system::error_code ec,
                const std::vector<
                    std::pair<std::string, std::vector<std::string>>>& Object) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG << "DBUS response error";
                    messages::internalError(aResp->res);
                    return;
                }

                for (const auto& [serviceName, interfaceList] : Object)
                {
                    crow::connections::systemBus->async_method_call(
                        [aResp, chassisID(std::string(chassisID))](
                            const boost::system::error_code ec2,
                            const std::vector<std::pair<
                                std::string, VariantType>>& propertiesList) {
                            if (ec2)
                            {
                                BMCWEB_LOG_DEBUG << "DBUS response error";
                                messages::internalError(aResp->res);
                                return;
                            }

                            nlohmann::json& tempArray =
                                aResp->res.jsonValue["Assemblies"];

                            std::string dataID = "/redfish/v1/Chassis/" +
                                                 chassisID +
                                                 "/Assembly#/Assemblies/";
                            dataID.append(std::to_string(tempArray.size()));

                            tempArray.push_back(
                                {{"@odata.type",
                                  "#Assembly.v1_3_0.AssemblyData"},
                                 {"@odata.id", dataID},
                                 {"MemberId", "0"},
                                 {"Name", "Assembly"}});

                            nlohmann::json& assemblyData = tempArray.back();

                            for (const std::pair<std::string, VariantType>&
                                     property : propertiesList)
                            {
                                if (property.first == "PartNumber")
                                {
                                    const std::string* value =
                                        std::get_if<std::string>(
                                            &property.second);
                                    if (value == nullptr)
                                    {
                                        messages::internalError(aResp->res);
                                        return;
                                    }
                                    assemblyData["PartNumber"] = *value;
                                }
                                else if (property.first == "SerialNumber")
                                {
                                    const std::string* value =
                                        std::get_if<std::string>(
                                            &property.second);
                                    if (value == nullptr)
                                    {
                                        messages::internalError(aResp->res);
                                        return;
                                    }
                                    assemblyData["SerialNumber"] = *value;
                                }
                                else if (property.first == "SparePartNumber")
                                {
                                    const std::string* value =
                                        std::get_if<std::string>(
                                            &property.second);
                                    if (value == nullptr)
                                    {
                                        messages::internalError(aResp->res);
                                        return;
                                    }
                                    assemblyData["SparePartNumber"] = *value;
                                }
                                else if (property.first == "Model")
                                {
                                    const std::string* value =
                                        std::get_if<std::string>(
                                            &property.second);
                                    if (value == nullptr)
                                    {
                                        messages::internalError(aResp->res);
                                        return;
                                    }
                                    assemblyData["Model"] = *value;
                                }
                                else if (property.first == "LocationCode")
                                {
                                    const std::string* value =
                                        std::get_if<std::string>(
                                            &property.second);
                                    if (value == nullptr)
                                    {
                                        messages::internalError(aResp->res);
                                        return;
                                    }
                                    assemblyData["Location"]["PartLocation"]
                                                ["ServiceLabel"] = *value;
                                }
                            }
                        },
                        serviceName, assembly,
                        "org.freedesktop.DBus.Properties", "GetAll", "");
                }
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetObject", assembly,
            std::array<const char*, 4>{
                "xyz.openbmc_project.Inventory.Item.Vrm",
                "xyz.openbmc_project.Inventory.Item.Tpm",
                "xyz.openbmc_project.Inventory.Item.Panel",
                "xyz.openbmc_project.Inventory.Item.Battery"});
    }
}

/**
 * @brief Get list of assemblies that are associated to the given
 * chassis.
 * @param[in] aResp - Shared pointer for asynchronous calls.
 * @param[in] chassisPath - Chassis to which the assemblies are
 * associated.
 *
 * @return None.
 */
inline void getAssemblyLinkedToChassis(const std::shared_ptr<AsyncResp>& aResp,
                                       const std::string* chassisPath)
{
    BMCWEB_LOG_DEBUG << "Get Assemblies associated to chassis";

    auto assemblyPath = *chassisPath + "/assembly";

    std::string chassis =
        sdbusplus::message::object_path(*chassisPath).filename();
    if (chassis.empty())
    {
        BMCWEB_LOG_ERROR << "Failed to find / in Chassis path";
        return;
    }

    crow::connections::systemBus->async_method_call(
        [aResp, chassis, assemblyPath(std::string(assemblyPath))](
            const boost::system::error_code ec,
            const std::variant<std::vector<std::string>>& endpoints) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }

            const std::vector<std::string>* assemblyList =
                std::get_if<std::vector<std::string>>(&(endpoints));

            if (assemblyList == nullptr)
            {
                BMCWEB_LOG_DEBUG << "No assembly found";
                messages::internalError(aResp->res);
                return;
            }

            getAssemblyProperties(aResp, *assemblyList, chassis);
        },
        "xyz.openbmc_project.ObjectMapper", assemblyPath,
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Association", "endpoints");
}

/**
 * @brief Get chassis path with given chassis ID
 * @param[in] aResp - Shared pointer for asynchronous calls.
 * @param[in] chassisID - Chassis to which the assemblies are
 * associated.
 *
 * @return None.
 */
inline void getChassis(const std::shared_ptr<AsyncResp>& aResp,
                       const std::string& chassisID)
{
    BMCWEB_LOG_DEBUG << "Get chassis path";

    // get the chassis path
    crow::connections::systemBus->async_method_call(
        [aResp, chassisID(std::string(chassisID))](
            const boost::system::error_code ec,
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
                std::string chassis =
                    sdbusplus::message::object_path(path).filename();
                if (chassis != chassisID)
                {
                    // this is not the chassis we are interested in
                    continue;
                }

                getAssemblyLinkedToChassis(aResp, &path);
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        "/xyz/openbmc_project/inventory", 0,
        std::array<const char*, 1>{
            "xyz.openbmc_project.Inventory.Item.Chassis"});
}

class Assembly : public Node
{
  public:
    /*
     * Default Constructor
     */
    Assembly(App& app) :
        Node(app, "/redfish/v1/Chassis/<str>/Assembly/", std::string())
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
     * Function to generate Assmely schema, This schema is used
     * to represent inventory items in Redfish when there is no
     * specific schema definition available
     */
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        if (params.size() != 1)
        {
            messages::internalError(res);

            res.end();
            return;
        }
        const std::string& chassisID = params[0];
        res.jsonValue["@odata.type"] = "#Assembly.v1_3_0.Assembly";
        res.jsonValue["@odata.id"] =
            "/redfish/v1/Chassis/" + chassisID + "/Assembly";
        res.jsonValue["Name"] = "Assembly Collection";
        res.jsonValue["Id"] = "Assembly info";

        auto asyncResp = std::make_shared<AsyncResp>(res);
        getChassis(asyncResp, chassisID);
    }
};

} // namespace redfish