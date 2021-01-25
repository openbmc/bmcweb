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

            for (const auto& [objectPath, serviceName] : subtree)
            {
                if (objectPath.empty() || serviceName.size() != 1)
                {
                    BMCWEB_LOG_DEBUG << "Error getting PCIeSlot D-Bus object!";
                    messages::internalError(asyncResp->res);
                    return;
                }
                const std::string& path = objectPath;
                const std::string& connectionName = serviceName[0].first;

                crow::connections::systemBus->async_method_call(
                    [asyncResp](
                        const boost::system::error_code ec,
                        const std::vector<
                            std::pair<std::string,
                                      std::variant<bool, size_t, std::string>>>&
                            propertiesList) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG
                                << "Can't get PCIeSlot properties!";
                            return;
                        }

                        const boost::container::flat_map<std::string,
                                                         std::string>
                            generationMaps = {{"xyz.openbmc_project.Inventory."
                                               "Item.PCIeSlot.Generations."
                                               "Gen1",
                                               "Gen1"},
                                              {"xyz.openbmc_project.Inventory."
                                               "Item.PCIeSlot.Generations."
                                               "Gen2",
                                               "Gen2"},
                                              {"xyz.openbmc_project.Inventory."
                                               "Item.PCIeSlot.Generations."
                                               "Gen3",
                                               "Gen3"},
                                              {"xyz.openbmc_project.Inventory."
                                               "Item.PCIeSlot.Generations."
                                               "Gen4",
                                               "Gen4"},
                                              {"xyz.openbmc_project.Inventory."
                                               "Item.PCIeSlot.Generations."
                                               "Gen5",
                                               "Gen5"},
                                              {"xyz.openbmc_project.Inventory."
                                               "Item.PCIeSlot.Generations."
                                               "Unknown",
                                               "Unknown"}};
                        const boost::container::flat_map<std::string,
                                                         std::string>
                            slotTypeMaps = {{"xyz.openbmc_project.Inventory."
                                             "Item.PCIeSlot.SlotTypes."
                                             "FullLength",
                                             "FullLength"},
                                            {"xyz.openbmc_project.Inventory."
                                             "Item.PCIeSlot.SlotTypes."
                                             "HalfLength",
                                             "HalfLength"},
                                            {"xyz.openbmc_project.Inventory."
                                             "Item.PCIeSlot.SlotTypes."
                                             "LowProfile",
                                             "LowProfile"},
                                            {"xyz.openbmc_project.Inventory."
                                             "Item.PCIeSlot.SlotTypes."
                                             "Mini",
                                             "Mini"},
                                            {"xyz.openbmc_project.Inventory."
                                             "Item.PCIeSlot.SlotTypes."
                                             "M_2",
                                             "M_2"},
                                            {"xyz.openbmc_project.Inventory."
                                             "Item.PCIeSlot.SlotTypes."
                                             "OEM",
                                             "OEM"},
                                            {"xyz.openbmc_project.Inventory."
                                             "Item.PCIeSlot.SlotTypes."
                                             "OCP3Small",
                                             "OCP3Small"},
                                            {"xyz.openbmc_project.Inventory."
                                             "Item.PCIeSlot.SlotTypes."
                                             "OCP3Large",
                                             "OCP3Large"},
                                            {"xyz.openbmc_project.Inventory."
                                             "Item.PCIeSlot.SlotTypes."
                                             "U_2",
                                             "U_2"},
                                            {"xyz.openbmc_project.Inventory."
                                             "Item.PCIeSlot.SlotTypes."
                                             "Unknown",
                                             "Unknown"}};

                        nlohmann::json& tempArray =
                            asyncResp->res.jsonValue["Slots"];
                        tempArray.push_back({});
                        nlohmann::json& propertyData = tempArray.back();

                        for (const std::pair<
                                 std::string,
                                 std::variant<bool, size_t, std::string>>&
                                 property : propertiesList)
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
                                auto generationMapsIt =
                                    generationMaps.find(*value);
                                if (generationMapsIt == generationMaps.end())
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }

                                propertyData["PCIeType"] =
                                    generationMapsIt->second;
                            }
                            else if ((propertyName == "Lanes"))
                            {
                                const size_t* value =
                                    std::get_if<size_t>(&property.second);
                                if (value == nullptr)
                                {
                                    // illegal property
                                    messages::internalError(asyncResp->res);
                                    continue;
                                }
                                propertyData[propertyName] = *value;
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
                                auto slotTypeMapsIt = slotTypeMaps.find(*value);
                                if (slotTypeMapsIt == slotTypeMaps.end())
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }

                                propertyData[propertyName] =
                                    slotTypeMapsIt->second;
                            }
                            else if ((propertyName == "HotPluggable"))
                            {
                                const bool* value =
                                    std::get_if<bool>(&property.second);
                                if (value == nullptr)
                                {
                                    // illegal property
                                    messages::internalError(asyncResp->res);
                                    continue;
                                }
                                propertyData[propertyName] = *value;
                            }
                        }
                    },
                    connectionName, path, "org.freedesktop.DBus.Properties",
                    "GetAll", "xyz.openbmc_project.Inventory.Item.PCIeSlot");
            }
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
            {"Name", "PCIe Slot Information"}};
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Chassis/" + chassisID + "/PCIeSlots";
        asyncResp->res.jsonValue["Id"] = "1";

        getPCIeSlots(asyncResp, chassisID);
    }
};

} // namespace redfish
