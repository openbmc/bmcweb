#pragma once

#include <node.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{
using MapperGetSubTreeResponse = std::vector<
    std::pair<std::string,
              std::vector<std::pair<std::string, std::vector<std::string>>>>>;

/**
 * @brief Retrieves properties over dbus for inventory items which
 * are to be published as assembly on a given chassis
 *
 * @param[in] aResp - Shared pointer for asynchronous calls.
 * @param[in] chassisID - Chassis to which the assemblies are
 * associated.
 *
 * @return None.
 */
inline void getAssemblies(const std::shared_ptr<AsyncResp>& aResp,
                          const std::string& chassisID)
{
    BMCWEB_LOG_DEBUG << "Get properties for assembly associated to chassis = "
                     << chassisID;

    aResp->res.jsonValue["Assemblies"] = nlohmann::json::array();

    // append chassisID to he rooth path of mapper to checkif the assembly is
    // associated to the given chassis ID
    std::string rootPath = "/xyz/openbmc_project/inventory/system/";
    rootPath.append(chassisID);

    crow::connections::systemBus->async_method_call(
        [aResp, chassisID(std::string(chassisID))](
            const boost::system::error_code ec,
            const MapperGetSubTreeResponse& subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }

            BMCWEB_LOG_DEBUG << "Number of assembly found = " << subtree.size();
            aResp->res.jsonValue["Assemblies@odata.count"] = subtree.size();

            for (const auto& [objectPath, serviceMap] : subtree)
            {
                for (const auto& [serviceName, interfaceList] : serviceMap)
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

                            BMCWEB_LOG_DEBUG << "Got " << propertiesList.size()
                                             << " properties for assembly";

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
                                        continue;
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
                                        continue;
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
                                        continue;
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
                                        continue;
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
                                        continue;
                                    }
                                    assemblyData["Location"]["PartLocation"]
                                                ["ServiceLabel"] = *value;
                                }
                            }
                        },
                        serviceName, objectPath,
                        "org.freedesktop.DBus.Properties", "GetAll", "");
                }
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", rootPath, 0,
        std::array<const char*, 4>{
            "xyz.openbmc_project.Inventory.Item.Vrm",
            "xyz.openbmc_project.Inventory.Item.Tpm",
            "xyz.openbmc_project.Inventory.Item.Panel",
            "xyz.openbmc_project.Inventory.Item.Battery",
        });
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
     * to represent inventory items in Redfish where there is no
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
        getAssemblies(asyncResp, chassisID);
    }
};

} // namespace redfish