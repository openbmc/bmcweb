#pragma once

#include "app.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/sw_utils.hpp"

#include <boost/lexical_cast.hpp>

#include <array>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

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
using OptionsItemType =
    std::tuple<std::string, std::variant<int64_t, std::string>>;

enum BiosBaseTableIndex
{
    biosBaseAttrType = 0,
    biosBaseReadonlyStatus,
    biosBaseDisplayName,
    biosBaseDescription,
    biosBaseMenuPath,
    biosBaseCurrValue,
    biosBaseDefaultValue,
    biosBaseOptions
};
enum OptionsItemIndex
{
    optItemType = 0,
    optItemValue
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
    pendingAttrType = 0,
    pendingAttrValue
};
static std::string mapAttrTypeToRedfish(const std::string_view typeDbus)
{
    std::string ret;
    if (typeDbus ==
        "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Enumeration")
    {
        ret = "Enumeration";
    }
    else if (typeDbus ==
             "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.String")
    {
        ret = "String";
    }
    else if (typeDbus ==
             "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Password")
    {
        ret = "Password";
    }
    else if (typeDbus ==
             "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Integer")
    {
        ret = "Integer";
    }
    else if (typeDbus ==
             "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Boolean")
    {
        ret = "Boolean";
    }
    else
    {
        ret = "UNKNOWN";
    }

    return ret;
}
static std::string mapBoundTypeToRedfish(const std::string_view typeDbus)
{
    std::string ret;
    if (typeDbus ==
        "xyz.openbmc_project.BIOSConfig.Manager.BoundType.ScalarIncrement")
    {
        ret = "ScalarIncrement";
    }
    else if (typeDbus ==
             "xyz.openbmc_project.BIOSConfig.Manager.BoundType.LowerBound")
    {
        ret = "LowerBound";
    }
    else if (typeDbus ==
             "xyz.openbmc_project.BIOSConfig.Manager.BoundType.UpperBound")
    {
        ret = "UpperBound";
    }
    else if (typeDbus ==
             "xyz.openbmc_project.BIOSConfig.Manager.BoundType.MinStringLength")
    {
        ret = "MinLength";
    }
    else if (typeDbus ==
             "xyz.openbmc_project.BIOSConfig.Manager.BoundType.MaxStringLength")
    {
        ret = "MaxLength";
    }
    else if (typeDbus ==
             "xyz.openbmc_project.BIOSConfig.Manager.BoundType.OneOf")
    {
        ret = "OneOf";
    }
    else
    {
        ret = "UNKNOWN";
    }

    return ret;
}

/**
 * BiosService class supports handle get method for bios.
 */
inline void
    handleBiosServiceGet(crow::App& app, const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (bmcwebEnableMultiHost)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Systems/system/Bios";
    asyncResp->res.jsonValue["@odata.type"] = "#Bios.v1_1_0.Bios";
    asyncResp->res.jsonValue["Name"] = "BIOS Configuration";
    asyncResp->res.jsonValue["Description"] = "BIOS Configuration Service";
    asyncResp->res.jsonValue["Id"] = "BIOS";
    asyncResp->res.jsonValue["Actions"]["#Bios.ResetBios"] = {
        {"target", "/redfish/v1/Systems/system/Bios/Actions/Bios.ResetBios"}};

    // Get the ActiveSoftwareImage and SoftwareImages
    sw_util::populateSoftwareInformation(asyncResp, sw_util::biosPurpose, "",
                                         true);
    asyncResp->res.jsonValue["@Redfish.Settings"] = {
        {"@odata.type", "#Settings.v1_3_0.Settings"},
        {"SettingsObject",
         {{"@odata.id", "/redfish/v1/Systems/system/Bios/Settings"}}}};
    asyncResp->res.jsonValue["AttributeRegistry"] = "BiosAttributeRegistry";
    asyncResp->res.jsonValue["Attributes"] = {};

    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::MapperGetObject& getObjectType) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("ObjectMapper::GetObject call failed: {} ", ec);
            messages::internalError(asyncResp->res);

            return;
        }
        const std::string& service = getObjectType.begin()->first;

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code& ec1,
                        const std::variant<BiosBaseTableType>& retBiosTable) {
            if (ec1)
            {
                BMCWEB_LOG_ERROR("getBiosAttributes DBUS error: {} ", ec1);
                messages::internalError(asyncResp->res);
                return;
            }
            const BiosBaseTableType* baseBiosTable =
                std::get_if<BiosBaseTableType>(&retBiosTable);
            nlohmann::json& attributesJson =
                asyncResp->res.jsonValue["Attributes"];
            if (baseBiosTable == nullptr)
            {
                BMCWEB_LOG_ERROR("baseBiosTable == nullptr");
                messages::internalError(asyncResp->res);
                return;
            }
            for (const BiosBaseTableItemType& item : *baseBiosTable)
            {
                const std::string& key = item.first;
                const std::string& itemType =
                    std::get<biosBaseAttrType>(item.second);
                std::string attrType = mapAttrTypeToRedfish(itemType);
                if (attrType == "String" || attrType == "Enumeration")
                {
                    const std::string* currValue = std::get_if<std::string>(
                        &std::get<biosBaseCurrValue>(item.second));
                    attributesJson.emplace(
                        key, currValue != nullptr ? *currValue : "");
                }
                else if (attrType == "Integer")
                {
                    const int64_t* currValue = std::get_if<int64_t>(
                        &std::get<biosBaseCurrValue>(item.second));
                    attributesJson.emplace(
                        key, currValue != nullptr ? *currValue : 0);
                }
                else
                {
                    BMCWEB_LOG_ERROR("Unsupported attribute type.");
                    messages::internalError(asyncResp->res);
                }
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

inline void requestRoutesBiosService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Bios/")
        .privileges(redfish::privileges::getBios)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleBiosServiceGet, std::ref(app)));
}

/**
 * BiosSettings class supports handle GET/PATCH method for
 * BIOS configuration pending settings.
 */
inline void requestRoutesBiosSettings(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Bios/Settings")
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }

        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/Bios/Settings";
        asyncResp->res.jsonValue["@odata.type"] = "#Bios.v1_1_0.Bios";
        asyncResp->res.jsonValue["Name"] = "Bios Settings";
        asyncResp->res.jsonValue["Id"] = "BiosSettings";
        asyncResp->res.jsonValue["AttributeRegistry"] = "BiosAttributeRegistry";
        nlohmann::json attributes(nlohmann::json::value_t::object);
        asyncResp->res.jsonValue["Attributes"] = attributes;

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code& ec,
                        const dbus::utility::MapperGetObject& getObjectType) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("ObjectMapper::GetObject call failed: {}", ec);
                messages::internalError(asyncResp->res);

                return;
            }
            std::string service = getObjectType.begin()->first;

            crow::connections::systemBus->async_method_call(
                [asyncResp](const boost::system::error_code& ec1,
                            const std::variant<PendingAttributesType>&
                                retPendingAttributes) {
                if (ec1)
                {
                    BMCWEB_LOG_WARNING("getBiosSettings DBUS error: {}", ec1);
                    messages::resourceNotFound(
                        asyncResp->res, "Systems/system/Bios", "Settings");
                    return;
                }
                const PendingAttributesType* pendingAttributes =
                    std::get_if<PendingAttributesType>(&retPendingAttributes);
                nlohmann::json& attributesJson =
                    asyncResp->res.jsonValue["Attributes"];
                if (pendingAttributes == nullptr)
                {
                    BMCWEB_LOG_ERROR("pendingAttributes == nullptr ");
                    messages::internalError(asyncResp->res);
                    return;
                }
                for (const PendingAttributesItemType& item : *pendingAttributes)
                {
                    const std::string& key = item.first;
                    const std::string& itemType =
                        std::get<pendingAttrType>(item.second);
                    std::string attrType = mapAttrTypeToRedfish(itemType);
                    if (attrType == "String" || attrType == "Enumeration")
                    {
                        const std::string* currValue = std::get_if<std::string>(
                            &std::get<pendingAttrValue>(item.second));
                        attributesJson.emplace(
                            key, currValue != nullptr ? *currValue : "");
                    }
                    else if (attrType == "Integer")
                    {
                        const int64_t* currValue = std::get_if<int64_t>(
                            &std::get<pendingAttrValue>(item.second));
                        attributesJson.emplace(
                            key, currValue != nullptr ? *currValue : 0);
                    }
                    else
                    {
                        BMCWEB_LOG_ERROR("Unsupported attribute type.");
                        messages::internalError(asyncResp->res);
                    }
                }
            },
                service, "/xyz/openbmc_project/bios_config/manager",
                "org.freedesktop.DBus.Properties", "Get",
                "xyz.openbmc_project.BIOSConfig.Manager", "PendingAttributes");
        },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetObject",
            "/xyz/openbmc_project/bios_config/manager",
            std::array<const char*, 0>());
    });

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Bios/Settings")
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::patch)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemName) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }

        if (systemName != "system")
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }
        nlohmann::json attrsJson;

        if (!redfish::json_util::readJsonPatch(req, asyncResp->res,
                                               "Attributes", attrsJson))
        {
            return;
        }

        if (attrsJson.is_array())
        {
            BMCWEB_LOG_WARNING(
                "The value for 'Attributes' is in a different format");
            messages::propertyValueFormatError(asyncResp->res, attrsJson.dump(),
                                               "Attributes");
            return;
        }

        crow::connections::systemBus->async_method_call(
            [asyncResp, attrsJson,
             systemName](const boost::system::error_code& ec,
                         const std::variant<BiosBaseTableType>& retBiosTable) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("getBiosAttributes DBUS error: {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }

            boost::container::flat_map<std::string,
                                       std::pair<bool, std::string>>
                biosAttrsType;
            const BiosBaseTableType* baseBiosTable =
                std::get_if<BiosBaseTableType>(&retBiosTable);

            if (baseBiosTable == nullptr)
            {
                BMCWEB_LOG_ERROR("baseBiosTable == nullptr ");
                messages::internalError(asyncResp->res);
                return;
            }

            for (const BiosBaseTableItemType& item : *baseBiosTable)
            {
                biosAttrsType.try_emplace(
                    item.first,
                    std::make_pair(
                        std::get<biosBaseReadonlyStatus>(item.second),
                        std::get<biosBaseAttrType>(item.second)));
            }

            PendingAttributesType pendingAttributes;
            for (const auto& attrItr : attrsJson.items())
            {
                std::string attrName = attrItr.key();

                if (attrName.empty())
                {
                    BMCWEB_LOG_WARNING("Attribute Name cannot be null");
                    messages::invalidObject(
                        asyncResp->res,
                        boost::urls::format(
                            "redfish/v1/Systems/systemName/Bios/Settings"));
                    return;
                }
                auto it = biosAttrsType.find(attrName);
                if (it == biosAttrsType.end())
                {
                    messages::propertyUnknown(asyncResp->res, attrName);
                    return;
                }

                bool biosAttrReadOnlyStatus = ((*it).second).first;
                if (biosAttrReadOnlyStatus)
                {
                    BMCWEB_LOG_WARNING(
                        "Attribute Type is ReadOnly. Patch failed!");
                    messages::propertyNotWritable(asyncResp->res, attrName);
                    return;
                }

                std::string biosAttrType = ((*it).second).second;
                if (biosAttrType.empty())
                {
                    BMCWEB_LOG_ERROR("Attribute type not found in BIOS Table");
                    messages::internalError(asyncResp->res);
                    return;
                }

                std::string biosRedfishAttrType =
                    mapAttrTypeToRedfish(biosAttrType);
                if (biosRedfishAttrType == "Integer")
                {
                    if (attrItr.value().type() !=
                        nlohmann::json::value_t::number_unsigned)
                    {
                        BMCWEB_LOG_WARNING("The value must be of type int");
                        std::string val =
                            boost::lexical_cast<std::string>(attrItr.value());
                        messages::propertyValueTypeError(asyncResp->res, val,
                                                         attrName);
                        return;
                    }
                    int64_t attrValue = attrItr.value();
                    pendingAttributes.emplace_back(std::make_pair(
                        attrName, std::make_tuple(biosAttrType, attrValue)));
                }
                else if (biosRedfishAttrType == "String" ||
                         biosRedfishAttrType == "Enumeration" ||
                         biosRedfishAttrType == "Password")
                {
                    if (attrItr.value().type() !=
                        nlohmann::json::value_t::string)
                    {
                        BMCWEB_LOG_WARNING("The value must be of type string");
                        std::string val =
                            boost::lexical_cast<std::string>(attrItr.value());
                        messages::propertyValueTypeError(asyncResp->res, val,
                                                         attrName);
                        return;
                    }
                    std::string attrValue = attrItr.value();
                    pendingAttributes.emplace_back(std::make_pair(
                        attrName, std::make_tuple(biosAttrType, attrValue)));
                }
                else if (biosRedfishAttrType == "Boolean")
                {
                    if (attrItr.value().type() !=
                        nlohmann::json::value_t::boolean)
                    {
                        BMCWEB_LOG_WARNING("The value must be of type bool");
                        std::string val =
                            boost::lexical_cast<std::string>(attrItr.value());
                        messages::propertyValueTypeError(asyncResp->res, val,
                                                         attrName);
                        return;
                    }
                    bool attrValue = attrItr.value();
                    pendingAttributes.emplace_back(std::make_pair(
                        attrName, std::make_tuple(biosAttrType, attrValue)));
                }
                else
                {
                    BMCWEB_LOG_ERROR("Attribute Type in BiosTable is Unknown");
                    messages::internalError(asyncResp->res);
                    return;
                }
            }

            crow::connections::systemBus->async_method_call(
                [asyncResp](const boost::system::error_code& ec1) {
                if (ec1)
                {
                    BMCWEB_LOG_ERROR("doPatch resp_handler got error: {}", ec1);
                    messages::internalError(asyncResp->res);
                    return;
                }
            },
                "xyz.openbmc_project.BIOSConfigManager",
                "/xyz/openbmc_project/bios_config/manager",
                "org.freedesktop.DBus.Properties", "Set",
                "xyz.openbmc_project.BIOSConfig.Manager", "PendingAttributes",
                std::variant<PendingAttributesType>(pendingAttributes));
        },
            "xyz.openbmc_project.BIOSConfigManager",
            "/xyz/openbmc_project/bios_config/manager",
            "org.freedesktop.DBus.Properties", "Get",
            "xyz.openbmc_project.BIOSConfig.Manager", "BaseBIOSTable");
    });
}

/**
 * BiosAttributeRegistry class supports handle get method for BIOS attribute
 * registry.
 */
inline void requestRoutesBiosAttributeRegistry(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Registries/BiosAttributeRegistry/BiosAttributeRegistry")
        .privileges({{"Login"}})
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Registries/BiosAttributeRegistry/"
            "BiosAttributeRegistry";
        asyncResp->res.jsonValue["@odata.type"] =
            "#AttributeRegistry.v1_3_2.AttributeRegistry";
        asyncResp->res.jsonValue["Name"] = "Bios Attribute Registry";
        asyncResp->res.jsonValue["Id"] = "BiosAttributeRegistry";
        asyncResp->res.jsonValue["RegistryVersion"] = "1.0.0";
        asyncResp->res.jsonValue["Language"] = "en";
        asyncResp->res.jsonValue["OwningEntity"] = "OpenBMC";
        asyncResp->res.jsonValue["RegistryEntries"]["Attributes"] =
            nlohmann::json::array();

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code& ec,
                        const dbus::utility::MapperGetObject& getObjectType) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("ObjectMapper::GetObject call failed: {}", ec);
                messages::internalError(asyncResp->res);

                return;
            }
            std::string service = getObjectType.begin()->first;

            crow::connections::systemBus->async_method_call(
                [asyncResp](
                    const boost::system::error_code& ec1,
                    const std::variant<BiosBaseTableType>& retBiosTable) {
                if (ec1)
                {
                    BMCWEB_LOG_WARNING(
                        "getBiosAttributeRegistry DBUS error: {}", ec1);
                    messages::resourceNotFound(asyncResp->res,
                                               "Registries/Bios", "Bios");
                    return;
                }
                const BiosBaseTableType* baseBiosTable =
                    std::get_if<BiosBaseTableType>(&retBiosTable);
                nlohmann::json& attributeArray =
                    asyncResp->res.jsonValue["RegistryEntries"]["Attributes"];
                if (baseBiosTable == nullptr)
                {
                    BMCWEB_LOG_ERROR("baseBiosTable == nullptr ");
                    messages::internalError(asyncResp->res);
                    return;
                }
                for (const BiosBaseTableItemType& item : *baseBiosTable)
                {
                    nlohmann::json optionsArray = nlohmann::json::array();
                    const std::string& itemType =
                        std::get<biosBaseAttrType>(item.second);
                    std::string attrType = mapAttrTypeToRedfish(itemType);
                    if (attrType == "UNKNOWN")
                    {
                        BMCWEB_LOG_ERROR("attrType == UNKNOWN");
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    nlohmann::json attributeItem;
                    attributeItem["AttributeName"] = item.first;
                    attributeItem["Type"] = attrType;
                    attributeItem["ReadOnly"] =
                        std::get<biosBaseReadonlyStatus>(item.second);
                    attributeItem["DisplayName"] =
                        std::get<biosBaseDisplayName>(item.second);
                    attributeItem["HelpText"] =
                        std::get<biosBaseDescription>(item.second);
                    if (!std::get<biosBaseMenuPath>(item.second).empty())
                    {
                        attributeItem["MenuPath"] =
                            std::get<biosBaseMenuPath>(item.second);
                    }

                    if (attrType == "String" || attrType == "Enumeration")
                    {
                        const std::string* currValue = std::get_if<std::string>(
                            &std::get<biosBaseCurrValue>(item.second));
                        const std::string* defValue = std::get_if<std::string>(
                            &std::get<biosBaseDefaultValue>(item.second));
                        if (currValue != nullptr && !currValue->empty())
                        {
                            attributeItem["CurrentValue"] = *currValue;
                        }
                        if (defValue != nullptr && !defValue->empty())
                        {
                            attributeItem["DefaultValue"] = *defValue;
                        }
                    }
                    else if (attrType == "Integer")
                    {
                        const int64_t* currValue = std::get_if<int64_t>(
                            &std::get<biosBaseCurrValue>(item.second));
                        const int64_t* defValue = std::get_if<int64_t>(
                            &std::get<biosBaseDefaultValue>(item.second));
                        attributeItem["CurrentValue"] =
                            currValue != nullptr ? *currValue : 0;
                        attributeItem["DefaultValue"] =
                            defValue != nullptr ? *defValue : 0;
                    }
                    else
                    {
                        BMCWEB_LOG_ERROR("Unsupported attribute type.");
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    const std::vector<OptionsItemType>& optionsVector =
                        std::get<biosBaseOptions>(item.second);
                    for (const OptionsItemType& optItem : optionsVector)
                    {
                        nlohmann::json optItemJson;
                        const std::string& strOptItemType =
                            std::get<optItemType>(optItem);
                        std::string optItemTypeRedfish =
                            mapBoundTypeToRedfish(strOptItemType);
                        if (optItemTypeRedfish == "UNKNOWN")
                        {
                            BMCWEB_LOG_ERROR("optItemTypeRedfish == UNKNOWN");
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        if (optItemTypeRedfish == "OneOf")
                        {
                            const std::string* currValue =
                                std::get_if<std::string>(
                                    &std::get<optItemValue>(optItem));
                            if (currValue != nullptr)
                            {
                                optItemJson["ValueName"] = *currValue;
                                optionsArray.push_back(optItemJson);
                            }
                        }
                        else
                        {
                            const int64_t* currValue = std::get_if<int64_t>(
                                &std::get<optItemValue>(optItem));
                            if (currValue != nullptr)
                            {
                                attributeItem[optItemTypeRedfish] = *currValue;
                            }
                        }
                    }

                    if (!optionsArray.empty())
                    {
                        attributeItem["Value"] = optionsArray;
                    }
                    attributeArray.push_back(attributeItem);
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
    });
}

/**
 * BiosReset class supports handle POST method for Reset bios.
 * The class retrieves and sends data directly to D-Bus.
 *
 * Function handles POST method request.
 * Analyzes POST body message before sends Reset request data to D-Bus.
 */
inline void
    handleBiosResetPost(crow::App& app, const crow::Request& req,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if constexpr (bmcwebEnableMultiHost)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    if (systemName != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code& ec) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("Failed to reset bios: {}", ec);
            messages::internalError(asyncResp->res);
            return;
        }
    },
        "org.open_power.Software.Host.Updater", "/xyz/openbmc_project/software",
        "xyz.openbmc_project.Common.FactoryReset", "Reset");
}

inline void requestRoutesBiosReset(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Bios/Actions/Bios.ResetBios/")
        .privileges(redfish::privileges::postBios)
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleBiosResetPost, std::ref(app)));
}

} // namespace redfish
