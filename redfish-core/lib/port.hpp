#pragma once

#include <node.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{

using MapperGetSubTreeResponse = std::vector<
    std::pair<std::string,
              std::vector<std::pair<std::string, std::vector<std::string>>>>>;

using ServiceMap =
    std::vector<std::pair<std::string, std::vector<std::string>>>;

/*
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
*/

inline void getPortProperties(const std::shared_ptr<AsyncResp>& aResp,
                              const std::string& objPath,
                              const ServiceMap& serviceMap)
{
    BMCWEB_LOG_DEBUG << "Getting Properties for port " << objPath;

    for (const auto& connection : serviceMap)
    {
        for (const auto& interface : connection.second)
        {
            if (interface == "xyz.openbmc_project.Inventory.Item")
            {
                crow::connections::systemBus->async_method_call(
                    [aResp](const boost::system::error_code ec,
                            const std::variant<std::string>& property) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG << "DBUS response error";
                            messages::internalError(aResp->res);
                            return;
                        }

                        const std::string* value =
                            std::get_if<std::string>(&property);
                        if (value == nullptr)
                        {
                            BMCWEB_LOG_DEBUG
                                << "Null value returned for pretty name";
                            messages::internalError(aResp->res);
                            return;
                        }
                        aResp->res.jsonValue["Name"] = *value;
                    },
                    connection.first, objPath,
                    "org.freedesktop.DBus.Properties", "Get", interface,
                    "PrettyName");
            }
            else if (interface == "com.ibm.ipzvpd.Location")
            //"xyz.openbmc_project.Inventory.Decorator.LocationCode")
            {
                crow::connections::systemBus->async_method_call(
                    [aResp](const boost::system::error_code ec,
                            const std::variant<std::string>& property) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG << "DBUS response error";
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

                        aResp->res.jsonValue["Location"]["PartLocation"]
                                            ["ServiceLabel"] = *value;
                    },
                    connection.first, objPath,
                    "org.freedesktop.DBus.Properties", "Get", interface,
                    "LocationCode");
            }
        }
    }
}

inline void getPort(const std::shared_ptr<AsyncResp>& aResp,
                    const std::string& portId, const std::string& adapterId)
{
    BMCWEB_LOG_DEBUG << "Get port = " << portId
                     << " on adapter = " << adapterId;

    crow::connections::systemBus->async_method_call(
        [aResp, portId, adapterId](const boost::system::error_code ec,
                                   const MapperGetSubTreeResponse& subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }

            for (const auto& [objectPath, serviceMap] : subtree)
            {
                BMCWEB_LOG_DEBUG << "Object Path = " << objectPath;
                if (objectPath.find(adapterId) != std::string::npos)
                {
                    BMCWEB_LOG_DEBUG << "Adapter Found";
                    std::string portName =
                        sdbusplus::message::object_path(objectPath).filename();

                    if (portName != portId)
                    {
                        // not the port we are interested in
                        continue;
                    }

                    BMCWEB_LOG_DEBUG << "Port Found" << portName;
                    getPortProperties(aResp, objectPath, serviceMap);
                    return;
                }
            }
            BMCWEB_LOG_ERROR << "Port not found";
            messages::resourceNotFound(aResp->res, "Port", portId);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0,
        std::array<const char*, 1>{
            "xyz.openbmc_project.Inventory.Item.Connector"});
}

inline void getPortCollection(const std::shared_ptr<AsyncResp>& aResp,
                              const std::string& collectionPath,
                              const std::string& adapterId,
                              const std::vector<const char*>& interfaces)
{
    BMCWEB_LOG_DEBUG << "Get collection members for: " << collectionPath;
    crow::connections::systemBus->async_method_call(
        [collectionPath, aResp,
         adapterId](const boost::system::error_code ec,
                    const std::vector<std::string>& objects) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }
            nlohmann::json& members = aResp->res.jsonValue["Members"];
            members = nlohmann::json::array();

            for (const auto& object : objects)
            {
                BMCWEB_LOG_DEBUG << "Object Path = " << object;
                if (object.find(adapterId) != std::string::npos)
                {
                    BMCWEB_LOG_DEBUG << "Object Path found = " << object;

                    sdbusplus::message::object_path path(object);
                    std::string leaf = path.filename();
                    if (leaf.empty())
                    {
                        continue;
                    }
                    std::string newPath = collectionPath;
                    newPath += '/';
                    newPath += leaf;
                    members.push_back({{"@odata.id", std::move(newPath)}});
                }
            }
            aResp->res.jsonValue["Members@odata.count"] = members.size();
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        "/xyz/openbmc_project/inventory", 0, interfaces);
}

class PortCollection : public Node
{
  public:
    /*
     * Default Constructor
     */
    PortCollection(App& app) :
        Node(app,
             "/redfish/v1/Systems/system/FabricAdapters/"
             "<str>/Ports/",
             std::string())
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
        if (params.size() != 1)
        {
            // param not recieved
            messages::internalError(res);
            res.end();
            return;
        }

        const std::string& adapterId = params[0];
        res.jsonValue["@odata.type"] = "#PortCollection.PortCollection";
        res.jsonValue["Name"] = "Port Collection";

        std::string dataId =
            "/redfish/v1/Systems/system/FabricAdapters/" + adapterId + "/Ports";
        res.jsonValue["@odata.id"] = dataId;

        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

        getPortCollection(asyncResp, dataId, adapterId,
                          {"xyz.openbmc_project.Inventory.Item.Connector"});
    }
};

class Port : public Node
{
  public:
    /*
     * Default Constructor
     */
    Port(App& app) :
        Node(app, "/redfish/v1/Systems/system/FabricAdapters/<str>/Ports/<str>",
             std::string(), std::string())
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
        if (params.size() != 2)
        {
            // param no recieved
            messages::internalError(res);
            res.end();
            return;
        }
        auto asyncResp = std::make_shared<AsyncResp>(res);

        const std::string& adapterId = params[0];
        const std::string& portId = params[1];

        const std::string& dataId =
            "/redfish/v1/Systems/system/FabricAdapters/" + adapterId +
            "/Ports/" + portId;

        res.jsonValue["@odata.type"] = "#Port.v1_3_0.Port";
        res.jsonValue["@odata.id"] = dataId;
        res.jsonValue["Id"] = "Port";

        getPort(asyncResp, portId, adapterId);
    }
}; // class port

} // namespace redfish
