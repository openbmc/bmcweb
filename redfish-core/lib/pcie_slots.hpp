#pragma once

#include <node.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{

inline void getPCIeSlots(const std::shared_ptr<AsyncResp>& asyncResp,
                         const std::string& chassisID)
{
    BMCWEB_LOG_DEBUG << "Get properties for PCIeSlots associated to chassis = "
                     << chassisID;

    asyncResp->res.jsonValue["Slots"] = nlohmann::json::array();
    crow::connections::systemBus->async_method_call(
        [asyncResp](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>&
                subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "D-Bus response error on GetSubTree " << ec;
                return;
            }
            if (subtree.size() == 0)
            {
                BMCWEB_LOG_DEBUG << "Can't find PCIeSlot D-Bus object!";
                return;
            }

            if (subtree[0].first.empty() || subtree[0].second.size() != 1)
            {
                BMCWEB_LOG_DEBUG << "Error getting PCIeSlot D-Bus object!";
                messages::internalError(asyncResp->res);
                return;
            }

            const std::string& path = subtree[0].first;
            const std::string& connectionName = subtree[0].second[0].first;

            crow::connections::systemBus->async_method_call(
                [asyncResp](
                    const boost::system::error_code ec,
                    const std::vector<
                        std::pair<std::string, std::variant<std::string>>>&
                        propertiesList) {
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG << "Can't get PCIeSlot properties!";
                        return;
                    }
                    for (const std::pair<std::string,
                                         std::variant<std::string>>& property :
                         propertiesList)
                    {
                        const std::string& propertyName = property.first;

                        if ((propertyName == "Generation"))
                        {
                            const std::string* value =
                                std::get_if<std::string>(&property.second);
                            if (value == nullptr)
                            {
                                // illegal property
                                messages::internalError(asyncResp->res);
                                continue;
                            }
                            asyncResp->res.jsonValue["PCIeType"] = *value;
                        }
                        else if ((propertyName == "Lanes"))
                        {
                            const std::string* value =
                                std::get_if<std::string>(&property.second);
                            if (value == nullptr)
                            {
                                // illegal property
                                messages::internalError(asyncResp->res);
                                continue;
                            }
                            asyncResp->res.jsonValue[propertyName] = *value;
                        }
                        else if ((propertyName == "SlotType"))
                        {
                            const std::string* value =
                                std::get_if<std::string>(&property.second);
                            if (value == nullptr)
                            {
                                // illegal property
                                messages::internalError(asyncResp->res);
                                continue;
                            }
                            asyncResp->res.jsonValue[propertyName] = *value;
                        }
                        else if ((propertyName == "HotPluggable"))
                        {
                            const std::string* value =
                                std::get_if<std::string>(&property.second);
                            if (value == nullptr)
                            {
                                // illegal property
                                messages::internalError(asyncResp->res);
                                continue;
                            }
                            asyncResp->res.jsonValue[propertyName] = *value;
                        }
                    }
                },
                connectionName, path, "org.freedesktop.DBus.Properties",
                "GetAll", "xyz.openbmc_project.Inventory.Item.PCIeSlot");
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", int32_t(0),
        std::array<const char*, 1>{
            "xyz.openbmc_project.Inventory.Item.PCIeSlot"});
}

class PCIeSlots : public Node
{
  public:
    /*
     * Default Constructor
     */
    PCIeSlots(App& app) :
        Node(app, "/redfish/v1/Chassis/<str>/PCIeSlots/", std::string())
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& chassisID = params[0];
        asyncResp->res.jsonValue = {
            {"@odata.type", "#PCIeSlots.v1_4_0.PCIeSlots"},
            {"Name", "PCIe slots Device Collection"}};
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Chassis/" + chassisID + "/PCIeSlots";
        asyncResp->res.jsonValue["Id"] = "1";

        getPCIeSlots(asyncResp, chassisID);
    }
};

} // namespace redfish