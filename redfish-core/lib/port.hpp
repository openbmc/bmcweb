#pragma once

#include <node.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{

inline void getLocatoon(const std::shared_ptr<AsyncResp>& aResp,
                        const std::string& portID)
{
    BMCWEB_LOG_DEBUG << "Get properties for port = " << portID;

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

            for (const auuto & [ objectPath, serviceMap ] : subtree)
            {
                // Ignore any objects which don't end with our desired port name
                if (!boost::ends_with(objectPath, portID))
                {
                    continue;
                }

                for (const auto& [service, interfaceList] : servcieMap)
                {
                    for (const auto& interface : interfaceList)
                    {
                        if (interface == "xyz.openbmc_project.Inventory."
                                         "Decorator.LocationCode")
                        {
                            crow::connections::systemBus->async_method_call(
                                [aResp](
                                    const boost::system::error_code ec,
                                    const std::variant<std::string>& property) {
                                    if (ec)
                                    {
                                        BMCWEB_LOG_DEBUG
                                            << "DBUS response error for "
                                               "Location";
                                        messages::internalError(aResp->res);
                                        return;
                                    }

                                    const std::string* value =
                                        std::get_if<std::string>(&property);
                                    if (value == nullptr)
                                    {
                                        // illegal value
                                        messages::internalError(aResp->res);
                                        return;
                                    }

                                    aResp->res
                                        .jsonValue["Location"]["PartLocation"]
                                                  ["ServiceLabel"] = *value;
                                },
                                service, objectPath,
                                "org.freedesktop.DBus.Properties", "Get",
                                "xyz.openbmc_project.Inventory.Decorator."
                                "LocationCode",
                                "LocationCode");
                        }
                    }
                }
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", rootPath, 0,
        std::array<const char*, 1>{
            "xyz.openbmc_project.Inventory.Item.Connector"});
}

class PortCollection : public Node
{
  public:
    /*
     * Default Constructor
     */
    ProcessorCollection(App& app) :
        Node(app, "/redfish/v1/Systems/system/Processors/") // update
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
               const std::vector<std::string>&) override
    {
        res.jsonValue["@odata.type"] = "#PortCollection.PortCollection";
        res.jsonValue["Name"] = "Port Collection";

        res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/Processors"; // update
        auto asyncResp = std::make_shared<AsyncResp>(res);

        collection_util::getCollectionMembers(
            asyncResp, "/redfish/v1/Systems/system/Processors", // update
            {"xyz.openbmc_project.Inventory.Item.Connector"});
    }
};

class Port : public node
{
  public:
    /*
     * Default Constructor
     */
    Assembly(App& app) :
        Node(app, "/redfish/v1/Systems/system/Port/<str>", std::string())
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
     * Function to generate Port schema.
     */
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        if (params.size() != 1)
        {
            // param no recieved
            messages::internalError(res);
            res.end();
            return;
        }
        auto asyncResp = std::make_shared<AsyncResp>(res);

        const std::string& portId = params[0];
        res.jsonValue["@odata.type"] = "#Port.v1_3_0.Port";
        res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/Port/" + portId;
        res.jsonValue["Name"] = "Port";
        res.jsonValue["Id"] = "Port info";

        getLocation(asyncResp, portId);
    }
}; // class port

} // namespace redfish
