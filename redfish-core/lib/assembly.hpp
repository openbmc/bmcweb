#pragma once

#include <node.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{
using MapperGetSubTreeResponse = std::vector<
    std::pair<std::string,
              std::vector<std::pair<std::string, std::vector<std::string>>>>>;

/*inline void getAssemblyLocation(std::shared_ptr<AsyncResp> aResp,
                                const std::string& service,
                                const std::string& objectPath,
                                nlohmann::json& assemblyItems)
{
    crow::connections::systemBus->async_method_call(
        [aResp, assemblyItems](const boost::system::error_code ec2,
                               const std::variant<std::string>& property) {
            if (ec2)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error for location info";
                messages::internalError(aResp->res);
                return;
            }

            const std::string* value = std::get_if<std::string>(&property);

            if (value == nullptr)
            {
                messages::internalError(aResp->res);
                return;
            }
            assemblyItems.push_back(
                {{"AssemblyData",
                  {"PartNumber",
                   {"PartLocation", {"ServiceLabel", (*value)}}}}});
        },
        service, objectPath, "org.freedesktop.DBus.Properties", "Get",
        "com.ibm.ipzvpd.Location", "LocationCode");
    //"xyz.openbmc_project.Inventory.Decorator.LocationCode"); */
//}

/*inline void getAssemblyAssetInfo(std::shared_ptr<AsyncResp> aResp,
                                 const std::string& service,
                                 const std::string& objectPath,
                                 nlohmann::json& assemblyItems)
{}

inline void getAssemblyData(std::shared_ptr<AsyncResp> aResp,
                            const std::string& assemblyID)
{
    BMCWEB_LOG_DEBUG << "Get assembly data";

    crow::connections::systemBus->async_method_call(
        [assemblyID,
         aResp{std::move(aResp)}](const boost::system::error_code ec,
                                  const MapperGetSubTreeResponse& subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }
            for (const auto& [objectPath, serviceMap] : subtree)
            {
                // Ignore any objects which don't end with our desired ID
                if (!boost::ends_with(objectPath, assemblyID))
                {
                    continue;
                }

                BMCWEB_LOG_DEBUG << "object Path = "<<objectPath;

                for (const auto& [serviceName, interfaceList] : serviceMap)
                {
                    for (const auto& interface : interfaceList)
                    {
                        if (interface ==
                            "xyz.openbmc_project.Inventory.Decorator.Asset")
                        {
                            getAssemblyAssetInfo(aResp, serviceName,
                                                 objectPath);
                        }
                        else if (interface ==
"com.ibm.ipzvpd.Location")//"xyz.openbmc_project.Inventory."
                                              //"Decorator.LocationCode")
                        {
                            getAssemblyLocation(aResp, serviceName, objectPath);
                        }
                    }
                }
                return;
            }

            // no object path found for this assembltID
            messages::resourceNotFound(aResp->res, "Assembly", assemblyID);
            return;
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0,
        std::array<const char*, 2>{
            "xyz.openbmc_project.Inventory.Decorator.Asset",
            "com.ibm.ipzvpd.Location",
            //"xyz.openbmc_project.Inventory.Decorator.LocationCode",
        });
}
*/
inline void getAssemblyData(std::shared_ptr<AsyncResp> aResp)
{
    BMCWEB_LOG_DEBUG << "Get List of assemblies";

    crow::connections::systemBus->async_method_call(
        [aResp{std::move(aResp)}](const boost::system::error_code ec,
                                  const MapperGetSubTreeResponse& subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }

            BMCWEB_LOG_DEBUG << "Size of tree" << subtree.size();

            for (const auto& [objectPath, serviceMap] : subtree)
            {
                BMCWEB_LOG_DEBUG << "object Path = " << objectPath;
                for (const auto& [serviceName, interfaceList] : serviceMap)
                {
                    BMCWEB_LOG_DEBUG << "get aseembly asset data";
                    crow::connections::systemBus->async_method_call(
                        [aResp{std::move(aResp)}](const boost::system::error_code ec2,
                                const std::vector<
                                    std::pair<std::string, VariantType>>&
                                    propertiesList) {
                            if (ec2)
                            {
                                BMCWEB_LOG_DEBUG
                                    << "DBUS response error for asset info";
                                messages::internalError(aResp->res);
                                return;
                            }

                            nlohmann::json& assemblyItems =
                                aResp->res.jsonValue["Assemblies"]["items"];
                            assemblyItems = nlohmann::json::array();

                            BMCWEB_LOG_DEBUG << "Got " << propertiesList.size()
                                             << " properties for assembly";

                            nlohmann::json& assemblyData = aResp->res.jsonValue["AssemblyData"];

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
                                  //  assemblyItems.push_back(
                                  //      {{"PartNumber", *value}});
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
                                   // assemblyItems.push_back(
                                   //     {{"SerialNumber", *value}});
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
                                  //  assemblyItems.push_back(
                                   //     {{"SparePartNumber", *value}});
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
                                   // assemblyItems.push_back(
                                     //   {{"SparePartNumber", *value}});
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
                                    assemblyData["Location"]["PartLocation"]["ServiceLAbel"] = *value;
                                   // nlohmann::json location;// = assemblyItems.back();
                                   // location["Location"]["PartLocation"]["ServiceLAbel"] = *value;
                                   // assemblyItems.push_back(location);
                                }
                            }

                            assemblyItems.push_back(assemblyData);
                        },
                        serviceName, objectPath,
                        "org.freedesktop.DBus.Properties", "GetAll", "");
                }
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0,
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
     * Functions triggers appropriate requests on DBus
     */
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        // Check if there is required param, truly entering this shall be
        // impossible
        if (params.size() != 1)
        {
            messages::internalError(res);

            res.end();
            return;
        }
        const std::string& chassisID = params[0];
        res.jsonValue["@odata.type"] = "#Assembly.v1_3_0.Assembly";
        res.jsonValue["@odata.id"] =
            "/redfish/v1/Chassis/" + chassisID + "/Assembly/";

        auto asyncResp = std::make_shared<AsyncResp>(res);
        getAssemblyData(asyncResp);
    }
};

} // namespace redfish