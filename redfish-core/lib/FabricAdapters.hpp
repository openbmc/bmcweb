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

/**
 * @brief Api to fetch and publish properties for the
 * given Fabric adapter.
 *
 * @param[in,out]   aResp       Async HTTP response.
 * @param[in]       adapter     Fabric adapter.
 * @param[in]       objPath     DBus path for the given Fabric adapter.
 * @param[in]       serviceMap  A map to hold services and Interface list
 * fetched for the given object path.
 */
inline void
    getAdapterProperties(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                         const std::string& adapter, const std::string& objPath,
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

/**
 * @brief Api to look for specific fabric adapter among
 * all available Fabric adapters on a system.
 *
 * @param[in,out]   aResp       Async HTTP response.
 * @param[in]       adapter     Fabric adapter to look for.
 */
inline void getAdapter(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                       const std::string& adapter)
{
    aResp->res.jsonValue["@odata.type"] = "#FabricAdapter.v1_0_0.FabricAdapter";
    aResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/Systems/system/FabricAdapters/" + adapter;

    crow::connections::systemBus->async_method_call(
        [adapter, aResp](const boost::system::error_code ec,
                         const MapperGetSubTreeResponse& subtree) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBus method call failed with error "
                                 << ec.value();

                // No adapter objects found by mapper
                if (ec.value() == boost::system::errc::io_error)
                {
                    messages::resourceNotFound(
                        aResp->res, "#FabricAdapter.v1_0_0.FabricAdapter",
                        adapter);
                    return;
                }

                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }
            for (const auto& [objectPath, serviceMap] : subtree)
            {
                std::string adapterId =
                    sdbusplus::message::object_path(objectPath).filename();

                if (adapterId.empty())
                {
                    BMCWEB_LOG_ERROR << "Failed to find / in adapter path";
                    messages::internalError(aResp->res);
                    return;
                }

                if (adapterId != adapter)
                {
                    // this is not the adapter we are interested in
                    continue;
                }

                aResp->res.jsonValue["Id"] = adapterId;

                getAdapterProperties(aResp, adapterId, objectPath, serviceMap);
                return;
            }
            BMCWEB_LOG_ERROR << "Adapter not found";
            messages::resourceNotFound(aResp->res, "FabricAdapter", adapter);
            return;
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
    void doGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const crow::Request&, const std::vector<std::string>&) override
    {
        asyncResp->res.jsonValue["@odata.type"] =
            "#FabricAdapterCollection.FabricAdapterCollection";
        asyncResp->res.jsonValue["Name"] = "Fabric adapter Collection";

        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/FabricAdapters";

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
     * Function to generate FabricAdapters schema.
     */
    void doGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const crow::Request&,
               const std::vector<std::string>& params) override
    {
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        const std::string& fabricAdapter = params[0];
        BMCWEB_LOG_DEBUG << "Adapter that we got called for " << fabricAdapter;

        getAdapter(asyncResp, fabricAdapter);
    }
};

} // namespace redfish