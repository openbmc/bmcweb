#pragma once

#include <node.hpp>
#include <utils/chassis_utils.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{

inline std::string analysisGeneration(const std::string& pcieType)
{
    if (pcieType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen1")
    {
        return "Gen1";
    }
    if (pcieType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen2")
    {
        return "Gen2";
    }
    if (pcieType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen3")
    {
        return "Gen3";
    }
    if (pcieType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen4")
    {
        return "Gen4";
    }
    if (pcieType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.Generations.Gen5")
    {
        return "Gen5";
    }

    // Unknown or others
    return "";
}

inline std::string analysisSlotType(const std::string& slotType)
{
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.FullLength")
    {
        return "FullLength";
    }
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.HalfLength")
    {
        return "HalfLength";
    }
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.LowProfile")
    {
        return "LowProfile";
    }
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.Mini")
    {
        return "Mini";
    }
    if (slotType == "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.M_2")
    {
        return "M2";
    }
    if (slotType == "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.OEM")
    {
        return "OEM";
    }
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.OCP3Small")
    {
        return "OCP3Small";
    }
    if (slotType ==
        "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.OCP3Large")
    {
        return "OCP3Large";
    }
    if (slotType == "xyz.openbmc_project.Inventory.Item.PCIeSlot.SlotTypes.U_2")
    {
        return "U2";
    }

    // Unknown or others
    return "";
}

inline void getPCIeSlots(const std::shared_ptr<AsyncResp>& asyncResp,
                         const std::string& chassisID)
{
    BMCWEB_LOG_DEBUG << "Get properties for PCIeSlots associated to chassis = "
                     << chassisID;

    asyncResp->res.jsonValue = {{"@odata.type", "#PCIeSlots.v1_4_1.PCIeSlots"},
                                {"Name", "PCIe Slot Information"}};
    asyncResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/Chassis/" + chassisID + "/PCIeSlots";
    asyncResp->res.jsonValue["Id"] = "PCIeSlots";
    asyncResp->res.jsonValue["Slots"] = nlohmann::json::array();

    crow::connections::systemBus->async_method_call(
        [asyncResp, chassisID](
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

                if (!boost::starts_with(
                        objectPath,
                        "/xyz/openbmc_project/inventory/system/" + chassisID))
                {
                    continue;
                }

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

                            if (propertyName == "Generation")
                            {
                                const std::string* value =
                                    std::get_if<std::string>(&property.second);
                                if (value == nullptr)
                                {
                                    // illegal property
                                    messages::internalError(asyncResp->res);
                                    return;
                                }

                                std::string pcieType =
                                    analysisGeneration(*value);
                                if (!pcieType.empty())
                                {
                                    propertyData["PCIeType"] = pcieType;
                                }
                            }
                            else if (propertyName == "Lanes")
                            {
                                const size_t* value =
                                    std::get_if<size_t>(&property.second);
                                if (value == nullptr)
                                {
                                    // illegal property
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                propertyData["Lanes"] = *value;
                            }
                            else if (propertyName == "SlotType")
                            {
                                const std::string* value =
                                    std::get_if<std::string>(&property.second);
                                if (value == nullptr)
                                {
                                    // illegal property
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                std::string slotType = analysisSlotType(*value);
                                if (!slotType.empty())
                                {
                                    propertyData["SlotType"] = slotType;
                                }
                            }
                            else if (propertyName == "HotPluggable")
                            {
                                const bool* value =
                                    std::get_if<bool>(&property.second);
                                if (value == nullptr)
                                {
                                    // illegal property
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                propertyData["HotPluggable"] = *value;
                            }
                        }
                    },
                    connectionName, objectPath,
                    "org.freedesktop.DBus.Properties", "GetAll",
                    "xyz.openbmc_project.Inventory.Item.PCIeSlot");
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

        auto getChassisID =
            [asyncResp,
             chassisID](const std::optional<std::string>& validChassisID) {
                if (!validChassisID)
                {
                    BMCWEB_LOG_ERROR << "Not a valid chassis ID:" << chassisID;
                    messages::resourceNotFound(asyncResp->res, "Chassis",
                                               chassisID);
                    return;
                }

                getPCIeSlots(asyncResp, chassisID);
            };
        redfish::chassis_utils::getValidChassisID(asyncResp, chassisID,
                                                  std::move(getChassisID));
    }
};

} // namespace redfish
