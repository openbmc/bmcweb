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
using BiosBaseTableType = std::vector<std::pair<
    std::string,
    std::tuple<
        std::string, bool, std::string, std::string, std::string,
        std::variant<int64_t, std::string>, std::variant<int64_t, std::string>,
        std::vector<
            std::tuple<std::string, std::variant<int64_t, std::string>>>>>>;
using BiosBaseTableItemType = std::pair<
    std::string,
    std::tuple<
        std::string, bool, std::string, std::string, std::string,
        std::variant<int64_t, std::string>, std::variant<int64_t, std::string>,
        std::vector<
            std::tuple<std::string, std::variant<int64_t, std::string>>>>>;

enum BiosBaseTableIndex
{
    BIOS_BASE_ATTR_TYPE = 0,
    BIOS_BASE_READONLY_STATUS,
    BIOS_BASE_DISPLAY_NAME,
    BIOS_BASE_DESCRIPTION,
    BIOS_BASE_MENU_PATH,
    BIOS_BASE_CURR_VALUE,
    BIOS_BASE_DEFAULT_VALUE,
    BIOS_BASE_OPTIONS
};

/*
 The Pending attribute name and new value.
              ex- { {"QuietBoot",Type.Integer, 0x1},
                    { "DdrFreqLimit",Type.String,"2933"}
                  }
*/
using PendingAttributesType = std::vector<std::pair<
    std::string, std::tuple<std::string, std::variant<int64_t, std::string>>>>;
using PendingAttributesItemType =
    std::pair<std::string,
              std::tuple<std::string, std::variant<int64_t, std::string>>>;
enum PendingAttributesIndex
{
    PENDING_ATTR_TYPE = 0,
    PENDING_ATTR_VALUE
};
static std::string mapTypeToRedfish(const std::string_view typeDbus)
{
    if (typeDbus == "xyz.openbmc_project.BIOSConfig.Manager."
                    "AttributeType.Enumeration")
    {
        return "Enumeration";
    }
    else if (typeDbus == "xyz.openbmc_project.BIOSConfig."
                         "Manager.AttributeType.String")
    {
        return "String";
    }
    else if (typeDbus == "xyz.openbmc_project.BIOSConfig."
                         "Manager.AttributeType.Password")
    {
        return "Password";
    }
    else if (typeDbus == "xyz.openbmc_project.BIOSConfig."
                         "Manager.AttributeType.Integer")
    {
        return "Integer";
    }
    else if (typeDbus == "xyz.openbmc_project.BIOSConfig."
                         "Manager.AttributeType.Boolean")
    {
        return "Boolean";
    }
    else
    {
        return "UNKNOWN";
    }
}

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
                        const GetObjectType& getObjectType) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "ObjectMapper::GetObject call failed: "
                                     << ec;
                    messages::internalError(asyncResp->res);

                    return;
                }
                std::string service = getObjectType.begin()->first;

                crow::connections::systemBus->async_method_call(
                    [asyncResp](
                        const boost::system::error_code ec,
                        const std::variant<BiosBaseTableType>& retBiosTable) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "getBiosAttributes DBUS error: "
                                             << ec;
                            messages::resourceNotFound(
                                asyncResp->res, "Systems/system", "Bios");
                            return;
                        }
                        const BiosBaseTableType* baseBiosTable =
                            std::get_if<BiosBaseTableType>(&retBiosTable);
                        nlohmann::json& attributesJson =
                            asyncResp->res.jsonValue["Attributes"];
                        if (baseBiosTable == nullptr)
                        {
                            BMCWEB_LOG_ERROR << "baseBiosTable == nullptr ";
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        for (const BiosBaseTableItemType& item : *baseBiosTable)
                        {
                            const std::variant<int64_t, std::string>& value =
                                std::get<BIOS_BASE_CURR_VALUE>(item.second);
                            const std::string& key = item.first;
                            attributesJson.emplace(key, value);
                        }
                    },
                    service, "/xyz/openbmc_project/bios_config/manager",
                    "org.freedesktop.DBus.Properties", "Get",
                    "xyz.openbmc_project.BIOSConfig.Manager", "BaseBIOSTable");
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetObject",
            "/xyz/openbmc_project/bios_config/manager",
            std::array<const char*, 0>());
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
            [asyncResp](const boost::system::error_code ec,
                        const GetObjectType& getObjectType) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "ObjectMapper::GetObject call failed: "
                                     << ec;
                    messages::internalError(asyncResp->res);

                    return;
                }
                std::string service = getObjectType.begin()->first;

                crow::connections::systemBus->async_method_call(
                    [asyncResp](const boost::system::error_code ec,
                                const std::variant<PendingAttributesType>&
                                    retPendingAttributes) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "getBiosSettings DBUS error: "
                                             << ec;
                            messages::resourceNotFound(asyncResp->res,
                                                       "Systems/system/Bios",
                                                       "Settings");
                            return;
                        }
                        const PendingAttributesType* pendingAttributes =
                            std::get_if<PendingAttributesType>(
                                &retPendingAttributes);
                        nlohmann::json& attributesJson =
                            asyncResp->res.jsonValue["Attributes"];
                        if (pendingAttributes == nullptr)
                        {
                            BMCWEB_LOG_ERROR << "pendingAttributes == nullptr ";
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        for (const PendingAttributesItemType& item :
                             *pendingAttributes)
                        {
                            const std::variant<int64_t, std::string>& value =
                                std::get<PENDING_ATTR_VALUE>(item.second);
                            const std::string& key = item.first;
                            attributesJson.emplace(key, value);
                        }
                    },
                    service, "/xyz/openbmc_project/bios_config/manager",
                    "org.freedesktop.DBus.Properties", "Get",
                    "xyz.openbmc_project.BIOSConfig.Manager",
                    "PendingAttributes");
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetObject",
            "/xyz/openbmc_project/bios_config/manager",
            std::array<const char*, 0>());
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
                        const GetObjectType& getObjectType) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "ObjectMapper::GetObject call failed: "
                                     << ec;
                    messages::internalError(asyncResp->res);

                    return;
                }
                std::string service = getObjectType.begin()->first;

                crow::connections::systemBus->async_method_call(
                    [asyncResp](
                        const boost::system::error_code ec,
                        const std::variant<BiosBaseTableType>& retBiosTable) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR
                                << "getBiosAttributeRegistry DBUS error: "
                                << ec;
                            messages::resourceNotFound(
                                asyncResp->res, "Registries/Bios", "Bios");
                            return;
                        }
                        const BiosBaseTableType* baseBiosTable =
                            std::get_if<BiosBaseTableType>(&retBiosTable);
                        nlohmann::json attributeArray = nlohmann::json::array();
                        if (baseBiosTable == nullptr)
                        {
                            BMCWEB_LOG_ERROR << "baseBiosTable == nullptr ";
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        for (const BiosBaseTableItemType& item : *baseBiosTable)
                        {
                            const std::string& itemType =
                                std::get<BIOS_BASE_ATTR_TYPE>(item.second);
                            std::string attrType = mapTypeToRedfish(itemType);
                            if (attrType == "UNKNOWN")
                            {
                                BMCWEB_LOG_ERROR << "attrType == UNKNOWN";
                                messages::internalError(asyncResp->res);
                            }
                            nlohmann::json attributeItem;
                            attributeItem.emplace("AttributeName", item.first);
                            attributeItem.emplace("Type", attrType);
                            attributeItem.emplace(
                                "ReadonlyStatus",
                                std::get<BIOS_BASE_READONLY_STATUS>(
                                    item.second));
                            attributeItem.emplace(
                                "DisplayName",
                                std::get<BIOS_BASE_DISPLAY_NAME>(item.second));
                            attributeItem.emplace(
                                "Description",
                                std::get<BIOS_BASE_DESCRIPTION>(item.second));
                            attributeItem.emplace(
                                "MenuPath",
                                std::get<BIOS_BASE_MENU_PATH>(item.second));
                            attributeItem.emplace(
                                "CurrentValue",
                                std::get<BIOS_BASE_CURR_VALUE>(item.second));
                            attributeItem.emplace(
                                "DefaultValue",
                                std::get<BIOS_BASE_DEFAULT_VALUE>(item.second));
                            attributeItem.emplace(
                                "Options",
                                std::get<BIOS_BASE_OPTIONS>(item.second));

                            attributeArray.push_back(attributeItem);
                        }
                        asyncResp->res
                            .jsonValue["RegistryEntries"]["Attributes"] =
                            attributeArray;
                    },
                    service, "/xyz/openbmc_project/bios_config/manager",
                    "org.freedesktop.DBus.Properties", "Get",
                    "xyz.openbmc_project.BIOSConfig.Manager", "BaseBIOSTable");
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetObject",
            "/xyz/openbmc_project/bios_config/manager",
            std::array<const char*, 0>());
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
