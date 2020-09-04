#pragma once

#include "node.hpp"

#include <utils/fw_utils.hpp>
namespace redfish
{

/*baseBIOSTable
map{attributeName,struct{attributeType,readonlyStatus,displayname,
              description,menuPath,current,default,
              array{struct{optionstring,optionvalue}}}}
*/
using biosBaseTable_t =
    std::map<std::string,
             std::tuple<std::string, bool, std::string, std::string,
                        std::string, std::variant<int64_t, std::string>,
                        std::variant<int64_t, std::string>,
                        std::vector<std::tuple<
                            std::string, std::variant<int64_t, std::string>>>>>;
using pendingAttributes_t =
    std::map<std::string,
             std::tuple<std::string, std::variant<int64_t, std::string>>>;
/**
 * BiosService class supports handle get method for bios.
 */
class BiosService : public Node
{
  public:
    BiosService(App& app) : Node(app, "/redfish/v1/Systems/system/Bios/")
    {
        entityPrivileges = {{boost::beast::http::verb::get, {{"Login"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>&) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);

        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/Bios";
        asyncResp->res.jsonValue["@odata.type"] = "#Bios.v1_1_0.Bios";
        asyncResp->res.jsonValue["Name"] = "BIOS Configuration";
        asyncResp->res.jsonValue["Description"] = "BIOS Configuration Service";
        asyncResp->res.jsonValue["Id"] = "BIOS";
        asyncResp->res.jsonValue["Actions"]["#Bios.ResetBios"] = {
            {"target",
             "/redfish/v1/Systems/system/Bios/Actions/Bios.ResetBios"}};

        // Get the ActiveSoftwareImage and SoftwareImages
        fw_util::populateFirmwareInformation(asyncResp, fw_util::biosPurpose,
                                             "", true);
        asyncResp->res.jsonValue["@Redfish.Settings"] = {
            {"@odata.type", "#Settings.v1_3_0.Settings"},
            {"SettingsObject",
             {{"@odata.id", "/redfish/v1/Systems/system/Bios/Settings"}}}};
        asyncResp->res.jsonValue["AttributeRegistry"] =
            "BiosAttributeRegistry.1.0.0";
        asyncResp->res.jsonValue["Attributes"] = {};
        getBiosAttributes(asyncResp);
    }
    void getBiosAttributes(std::shared_ptr<AsyncResp> asyncResp)
    {
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const std::variant<biosBaseTable_t>& retBiosTable) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "getBiosAttributes DBUS error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                const biosBaseTable_t* baseBiosTable =
                    std::get_if<biosBaseTable_t>(&retBiosTable);
                std::map<std::string, std::variant<int64_t, std::string>>
                    attributesMap;
                if (baseBiosTable == nullptr)
                {
                    BMCWEB_LOG_ERROR << "baseBiosTable == nullptr ";
                    messages::internalError(asyncResp->res);
                    return;
                }
                for (auto const& item : *baseBiosTable)
                {
                    auto& value = std::get<5>(item.second);
                    auto& key = item.first;
                    attributesMap.emplace(key, value);
                }
                nlohmann::json attributesJson(attributesMap);
                asyncResp->res.jsonValue["Attributes"] = attributesJson;
            },
            "xyz.openbmc_project.BIOSConfigManager",
            "/xyz/openbmc_project/bios_config/manager",
            "org.freedesktop.DBus.Properties", "Get",
            "xyz.openbmc_project.BIOSConfig.Manager", "BaseBIOSTable");
    }
};

/**
 * BiosSettings class supports handle GET/PATCH method for
 * BIOS configuration pending settings.
 */
class BiosSettings : public Node
{
  public:
    BiosSettings(App& app) :
        Node(app, "/redfish/v1/Systems/system/Bios/Settings")
    {
        entityPrivileges = {{boost::beast::http::verb::get, {{"Login"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>&) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/Bios/Settings";
        asyncResp->res.jsonValue["@odata.type"] = "#Bios.v1_1_0.Bios";
        asyncResp->res.jsonValue["Name"] = "Bios Settings Version 1";
        asyncResp->res.jsonValue["Id"] = "BiosSettingsV1";
        asyncResp->res.jsonValue["AttributeRegistry"] =
            "BiosAttributeRegistry.1.0.0";
        asyncResp->res.jsonValue["Attributes"] = {};

        getBiosSettings(asyncResp);
    }
    /**
     * Get BIOS Settings from d-bus.
     */
    void getBiosSettings(std::shared_ptr<AsyncResp> asyncResp)
    {
        crow::connections::systemBus->async_method_call(
            [asyncResp](
                const boost::system::error_code ec,
                const std::variant<pendingAttributes_t>& retPendingAttributes) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "getBiosSettings DBUS error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                const pendingAttributes_t* pendingAttributes =
                    std::get_if<pendingAttributes_t>(&retPendingAttributes);
                std::map<std::string, std::variant<int64_t, std::string>>
                    attributesMap;
                if (pendingAttributes == nullptr)
                {
                    BMCWEB_LOG_ERROR << "pendingAttributes == nullptr ";
                    messages::internalError(asyncResp->res);
                    return;
                }
                for (auto const& item : *pendingAttributes)
                {
                    auto& value = std::get<1>(item.second);
                    auto& key = item.first;
                    attributesMap.emplace(key, value);
                }
                nlohmann::json attributesJson(attributesMap);
                asyncResp->res.jsonValue["Attributes"] = attributesJson;
            },
            "xyz.openbmc_project.BIOSConfigManager",
            "/xyz/openbmc_project/bios_config/manager",
            "org.freedesktop.DBus.Properties", "Get",
            "xyz.openbmc_project.BIOSConfig.Manager", "PendingAttributes");
    }
};
/**
 * BiosAttributeRegistry class supports handle get method for BIOS attribute
 * registry.
 */
class BiosAttributeRegistry : public Node
{
  public:
    BiosAttributeRegistry(App& app) :
        Node(app, "/redfish/v1/Registries/Bios/Bios")
    {
        entityPrivileges = {{boost::beast::http::verb::get, {{"Login"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>&) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Registries/Bios/Bios";
        asyncResp->res.jsonValue["@odata.type"] =
            "#AttributeRegistry.v1_3_2.AttributeRegistry";
        asyncResp->res.jsonValue["Name"] = "Bios Attribute Registry";
        asyncResp->res.jsonValue["Id"] = "BiosAttributeRegistry";
        asyncResp->res.jsonValue["RegistryVersion"] = "1.0.0";
        asyncResp->res.jsonValue["Language"] = "en";
        asyncResp->res.jsonValue["OwningEntity"] = "OpenBMC";
        asyncResp->res.jsonValue["SupportedSystems"] = {};

        getBiosAttributeRegistry(asyncResp);
    }
    /**
     * Get BIOS attribute registry from d-bus.
     */
    void getBiosAttributeRegistry(std::shared_ptr<AsyncResp> asyncResp)
    {
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const std::variant<biosBaseTable_t>& retBiosTable) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "getBiosAttributeRegistry DBUS error: "
                                     << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                const biosBaseTable_t* baseBiosTable =
                    std::get_if<biosBaseTable_t>(&retBiosTable);
                nlohmann::json& attributeArray =
                    asyncResp->res.jsonValue["RegistryEntries"]["Attributes"];
                attributeArray = nlohmann::json::array();
                if (baseBiosTable == nullptr)
                {
                    BMCWEB_LOG_ERROR << "baseBiosTable == nullptr ";
                    messages::internalError(asyncResp->res);
                    return;
                }
                for (auto const& item : *baseBiosTable)
                {
                    std::string attrType = "UNKNOWN";
                    auto& itemType = std::get<0>(item.second);
                    if (itemType == "xyz.openbmc_project.BIOSConfig.Manager."
                                    "AttributeType.Enumeration")
                    {
                        attrType = "Enumeration";
                    }
                    else if (itemType == "xyz.openbmc_project.BIOSConfig."
                                         "Manager.AttributeType.String")
                    {
                        attrType = "String";
                    }
                    else if (itemType == "xyz.openbmc_project.BIOSConfig."
                                         "Manager.AttributeType.Password")
                    {
                        attrType = "Password";
                    }
                    else if (itemType == "xyz.openbmc_project.BIOSConfig."
                                         "Manager.AttributeType.Integer")
                    {
                        attrType = "Integer";
                    }
                    else if (itemType == "xyz.openbmc_project.BIOSConfig."
                                         "Manager.AttributeType.Boolean")
                    {
                        attrType = "Boolean";
                    }
                    attributeArray.push_back(
                        {{"AttributeName", item.first},
                         {"Type", attrType},
                         {"ReadonlyStatus", std::get<1>(item.second)},
                         {"DisplayName", std::get<2>(item.second)},
                         {"Discription", std::get<3>(item.second)},
                         {"MenuPath", std::get<4>(item.second)},
                         {"CurrentValue", std::get<5>(item.second)},
                         {"DefaultValue", std::get<6>(item.second)},
                         {"Options", std::get<7>(item.second)}});
                }
            },
            "xyz.openbmc_project.BIOSConfigManager",
            "/xyz/openbmc_project/bios_config/manager",
            "org.freedesktop.DBus.Properties", "Get",
            "xyz.openbmc_project.BIOSConfig.Manager", "BaseBIOSTable");
    }
};
/**
 * BiosReset class supports handle POST method for Reset bios.
 * The class retrieves and sends data directly to D-Bus.
 */
class BiosReset : public Node
{
  public:
    BiosReset(App& app) :
        Node(app, "/redfish/v1/Systems/system/Bios/Actions/Bios.ResetBios/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    /**
     * Function handles POST method request.
     * Analyzes POST body message before sends Reset request data to D-Bus.
     */
    void doPost(crow::Response& res, const crow::Request&,
                const std::vector<std::string>&) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Failed to reset bios: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
            },
            "org.open_power.Software.Host.Updater",
            "/xyz/openbmc_project/software",
            "xyz.openbmc_project.Common.FactoryReset", "Reset");
    }
};
} // namespace redfish
