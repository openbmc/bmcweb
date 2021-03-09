#pragma once

#include <node.hpp>
#include <utils/collection.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{

using MapperGetSubTreeResponse = std::vector<
    std::pair<std::string,
              std::vector<std::pair<std::string, std::vector<std::string>>>>>;

using ServiceMap =
    std::vector<std::pair<std::string, std::vector<std::string>>>;

inline void getAdapterProperties(std::shared_ptr<AsyncResp> aResp,
                                 const std::string& adapter,
                                 const std::string& objPath,
                                 const ServiceMap& serviceMap)
{
    BMCWEB_LOG_DEBUG << "Get adapter properties for " << adapter;

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
        }
    }
}

inline void getAdapter(std::shared_ptr<AsyncResp> aResp,
                       const std::string& adapter)
{
    BMCWEB_LOG_DEBUG << "Get fabric adapter data for adapter " << adapter;

    aResp->res.jsonValue["@odata.type"] = "#FabricAdapter.v1_0_0.FabricAdapter";
    aResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/Systems/system/FabricAdapters/" + adapter;

    crow::connections::systemBus->async_method_call(
        [adapter, aResp](const boost::system::error_code ec,
                         const MapperGetSubTreeResponse& subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }
            for (const auto& [objectPath, serviceMap] : subtree)
            {
                /*test log remove from file code*/
                BMCWEB_LOG_ERROR << "adapter paths that we got" << objectPath;

                std::string adapterId =
                    sdbusplus::message::object_path(objectPath).filename();
                if (adapterId.empty())
                {
                    BMCWEB_LOG_ERROR << "Failed to find / in adapter path";
                    messages::internalError(aResp->res);
                    return;
                }

                /*test log remove from file code*/
                BMCWEB_LOG_ERROR << "adapterId" << adapterId;

                if (adapterId != adapter)
                {
                    // this is not the adapter we are interested in
                    continue;
                }

                /*test log remove from file code*/
                BMCWEB_LOG_ERROR << "adapter path that we need" << objectPath;
                aResp->res.jsonValue["Id"] = adapterId;

                getAdapterProperties(aResp, adapterId, objectPath, serviceMap);
                return;
            }
            BMCWEB_LOG_ERROR << "Adapter not found";
            messages::resourceNotFound(aResp->res, "FabricAdapter", adapter);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0,
        std::array<const char*, 1>{
            "xyz.openbmc_project.Inventory.Item.FabricAdapter"});
}

class FabricAdapterCollection : public Node
{
  public:
    /*
     * Default Constructor
     */
    FabricAdapterCollection(App& app) :
        Node(app, "/redfish/v1/Systems/system/FabricAdapters/")
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
        res.jsonValue["@odata.type"] =
            "#FabricAdapterCollection.FabricAdapterCollection";
        res.jsonValue["Name"] = "Fabric adapter Collection";

        res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/FabricAdapters";
        auto asyncResp = std::make_shared<AsyncResp>(res);

        collection_util::getCollectionMembers(
            asyncResp, "/redfish/v1/Systems/system/FabricAdapters",
            {"xyz.openbmc_project.Inventory.Item.FabricAdapter"});
    }
};

class FabricAdapters : public Node
{
  public:
    /*
     * Default Constructor
     */
    FabricAdapters(App& app) :
        Node(app, "/redfish/v1/Systems/system/FabricAdapters/<str>/",
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
     * Function to generate Assembly schema, This schema is used
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
        const std::string& fabricAdapter = params[0];
        BMCWEB_LOG_DEBUG << "Adapter that we got called for " << fabricAdapter;
        auto asyncResp = std::make_shared<AsyncResp>(res);

        getAdapter(asyncResp, fabricAdapter);
    }
};

} // namespace redfish