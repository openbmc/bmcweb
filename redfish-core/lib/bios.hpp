#pragma once

#include <app.hpp>
#include <query.hpp>
#include <registries/privilege_registry.hpp>
#include <utils/sw_utils.hpp>

namespace redfish
{
using BiosBaseTableType = boost::container::flat_map<
    std::string,
    std::tuple<
        std::string, bool, std::string, std::string, std::string,
        std::variant<int64_t, std::string>, std::variant<int64_t, std::string>,
        std::vector<
            std::tuple<std::string, std::variant<int64_t, std::string>>>>>;

using BiosBaseTableItemType = std::pair<
    std::string,
    std::tuple<
        std::string, bool, std::string, std::string, std::string,
        std::variant<int64_t, std::string>, std::variant<int64_t, std::string>,
        std::vector<
            std::tuple<std::string, std::variant<int64_t, std::string>>>>>;
using OptionsItemType =
    std::tuple<std::string, std::variant<int64_t, std::string>>;

using PendingAttributesType = boost::container::flat_map<
    std::string, std::tuple<std::string, std::variant<int64_t, std::string>>>;

using PendingAttributesItemType =
    std::pair<std::string,
              std::tuple<std::string, std::variant<int64_t, std::string>>>;

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

enum PendingAttributesIndex
{
    pendingAttrType = 0,
    pendingAttrValue
};
static std::string mapAttrTypeToRedfish(const std::string_view typeDbus)
{
    std::string ret;
    if (typeDbus == "xyz.openbmc_project.BIOSConfig."
                    "Manager.AttributeType.String")
    {
        ret = "String";
    }
    else if (typeDbus == "xyz.openbmc_project.BIOSConfig."
                         "Manager.AttributeType.Integer")
    {
        ret = "Integer";
    }
    else if (typeDbus == "xyz.openbmc_project.BIOSConfig."
                         "Manager.AttributeType.Enumeration")
    {
        ret = "Enumeration";
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
        ret = "MinStringLength";
    }
    else if (typeDbus ==
             "xyz.openbmc_project.BIOSConfig.Manager.BoundType.MaxStringLength")
    {
        ret = "MaxStringLength";
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
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
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
}

inline void requestRoutesBiosService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/Bios/")
        .privileges(redfish::privileges::getBios)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleBiosServiceGet, std::ref(app)));
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
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "Failed to reset bios: " << ec;
            messages::internalError(asyncResp->res);
            return;
        }
        },
        "org.open_power.Software.Host.Updater", "/xyz/openbmc_project/software",
        "xyz.openbmc_project.Common.FactoryReset", "Reset");
}

inline void requestRoutesBiosReset(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/Bios/Actions/Bios.ResetBios/")
        .privileges(redfish::privileges::postBios)
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleBiosResetPost, std::ref(app)));
}

inline void handleBiosAttributeRegistryGet(
    const crow::Request& /* unused */,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
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
        [asyncResp](const boost::system::error_code ec,
                    const dbus::utility::MapperGetObject& getObjectType) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "ObjectMapper::GetObject call failed: " << ec;
            messages::internalError(asyncResp->res);

            return;
        }

        if (getObjectType.empty())
        {
            BMCWEB_LOG_ERROR << "getObjectType is empty.";
            messages::internalError(asyncResp->res);

            return;
        }

        std::string service = getObjectType.begin()->first;

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code errorCode,
                        const std::variant<BiosBaseTableType>& retBiosTable) {
            if (errorCode)
            {
                BMCWEB_LOG_ERROR << "getBiosAttributeRegistry DBUS error: "
                                 << errorCode;
                messages::resourceNotFound(asyncResp->res, "Registries/Bios",
                                           "Bios");
                return;
            }
            const BiosBaseTableType* baseBiosTable =
                std::get_if<BiosBaseTableType>(&retBiosTable);
            nlohmann::json& attributeArray =
                asyncResp->res.jsonValue["RegistryEntries"]["Attributes"];
            if (baseBiosTable == nullptr)
            {
                BMCWEB_LOG_ERROR << "baseBiosTable == nullptr ";
                messages::internalError(asyncResp->res);
                return;
            }
            for (const BiosBaseTableItemType& item : *baseBiosTable)
            {
                const std::string& itemType =
                    std::get<biosBaseAttrType>(item.second);
                std::string attrType = mapAttrTypeToRedfish(itemType);
                if (attrType == "UNKNOWN")
                {
                    BMCWEB_LOG_ERROR << "UNKNOWN attrType == " << itemType;
                    continue;
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
                attributeItem["MenuPath"] =
                    std::get<biosBaseMenuPath>(item.second);

                if (attrType == "String" || attrType == "Enumeration")
                {
                    const std::string* currValue = std::get_if<std::string>(
                        &std::get<biosBaseCurrValue>(item.second));

                    if (currValue == nullptr)
                    {
                        BMCWEB_LOG_ERROR << "Unable to get currValue, no "
                                            "std::string data in BIOS "
                                            "attributes item data";
                        continue;
                    }

                    const std::string* defValue = std::get_if<std::string>(
                        &std::get<biosBaseDefaultValue>(item.second));

                    if (defValue == nullptr)
                    {
                        BMCWEB_LOG_ERROR << "Unable to get defValue, no "
                                            "std::string data in BIOS "
                                            "attributes item data";
                        continue;
                    }

                    attributeItem["CurrentValue"] =
                        currValue != nullptr ? *currValue : "";
                    attributeItem["DefaultValue"] =
                        defValue != nullptr ? *defValue : "";
                }
                else if (attrType == "Integer")
                {
                    const int64_t* currValue = std::get_if<int64_t>(
                        &std::get<biosBaseCurrValue>(item.second));

                    if (currValue == nullptr)
                    {
                        BMCWEB_LOG_ERROR << "Unable to get currValue, no "
                                            "int64_t data in BIOS "
                                            "attributes item data";
                        continue;
                    }

                    const int64_t* defValue = std::get_if<int64_t>(
                        &std::get<biosBaseDefaultValue>(item.second));

                    if (defValue == nullptr)
                    {
                        BMCWEB_LOG_ERROR << "Unable to get defValue, no "
                                            "int64_t data in BIOS "
                                            "attributes item data";
                        continue;
                    }

                    attributeItem["CurrentValue"] =
                        currValue != nullptr ? *currValue : 0;
                    attributeItem["DefaultValue"] =
                        defValue != nullptr ? *defValue : 0;
                }
                else
                {
                    BMCWEB_LOG_ERROR << "UNKNOWN attrType == " << itemType;
                    continue;
                }

                nlohmann::json optionsArray = nlohmann::json::array();
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
                        BMCWEB_LOG_ERROR << "UNKNOWN optItemTypeRedfish == "
                                         << strOptItemType;
                        continue;
                    }
                    if (optItemTypeRedfish == "OneOf")
                    {
                        const std::string* currValue = std::get_if<std::string>(
                            &std::get<optItemValue>(optItem));

                        if (currValue == nullptr)
                        {
                            BMCWEB_LOG_ERROR << "Unable to get currValue, "
                                                "no "
                                                "std::string data in option "
                                                "item value";
                            continue;
                        }

                        optItemJson["ValueDisplayName"] =
                            currValue != nullptr ? *currValue : "";
                        optItemJson["ValueName"] =
                            currValue != nullptr ? *currValue : "";
                    }
                    else
                    {
                        const int64_t* currValue = std::get_if<int64_t>(
                            &std::get<optItemValue>(optItem));

                        if (currValue == nullptr)
                        {
                            BMCWEB_LOG_ERROR << "Unable to get currValue, "
                                                "no "
                                                "int64_t data in option "
                                                "item value";
                            continue;
                        }

                        optItemJson["ValueDisplayName"] =
                            currValue != nullptr ? *currValue : 0;
                        optItemJson["ValueName"] =
                            currValue != nullptr ? *currValue : 0;
                    }

                    optionsArray.push_back(optItemJson);
                }

                if (optionsArray.empty())
                {
                    BMCWEB_LOG_ERROR << "optionsArray is empty";
                    continue;
                }

                attributeItem["Value"] = optionsArray;
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
}

/**
 * BiosAttributeRegistry class supports handle get method for BIOS attribute
 * registry.
 */
inline void requestRoutesBiosAttributeRegistry(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Registries/BiosAttributeRegistry/BiosAttributeRegistry/")
        .privileges(redfish::privileges::getBios)
        .methods(boost::beast::http::verb::get)(handleBiosAttributeRegistryGet);
}

} // namespace redfish
