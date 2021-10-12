#pragma once

#include <app.hpp>
#include <registries/privilege_registry.hpp>
#include <utils/fw_utils.hpp>
namespace redfish
{
namespace bios
{
/* List of BIOS knobs */
using BaseTableType = boost::container::flat_map<
    std::string,
    std::tuple<
        std::string, bool, std::string, std::string, std::string,
        std::variant<int64_t, std::string>, std::variant<int64_t, std::string>,
        std::vector<
            std::tuple<std::string, std::variant<int64_t, std::string>>>>>;

/* One BIOS knob */
using BaseTableItemType = std::pair<
    std::string,
    std::tuple<
        std::string, bool, std::string, std::string, std::string,
        std::variant<int64_t, std::string>, std::variant<int64_t, std::string>,
        std::vector<
            std::tuple<std::string, std::variant<int64_t, std::string>>>>>;

/* Options inside a BIOS knob */
using OptionsItemType =
    std::tuple<std::string, std::variant<int64_t, std::string>>;

/**
 * BaseTableItemIndex is used to access elements inside BaseTableItemType,
 */
enum BaseTableItemIndex
{
    AttrType = 0,
    ReadonlyStatus,
    DisplayName,
    Description,
    MenuPath,
    CurrValue,
    DefaultValue,
    Options
};

/**
 * OptionsItemIndex is used to access elements inside OptionsItemType,
 */
enum OptionsItemIndex
{
    Type = 0,
    Value
};

/**
 * populateJsonTypeInt64 adds one key value (int64_t)
 * pair to user provided json.
 */
bool populateJsonTypeInt64(nlohmann::json& src, const std::string& key,
                           const std::variant<int64_t, std::string>& value,
                           const int64_t& defaultValue = 0)
{
    const int64_t* pValue = std::get_if<int64_t>(&value);
    if (!pValue)
    {
        BMCWEB_LOG_ERROR << "Unable to get value, no int64_t data in variant, "
                            "hence using default value";
        src[key] = defaultValue;
        return false;
    }

    src[key] = *pValue;

    return true;
}

/**
 * populateJsonTypeString adds one key value (std::string)
 * pair to user provided json.
 */
bool populateJsonTypeString(nlohmann::json& src, const std::string& key,
                            const std::variant<int64_t, std::string>& value,
                            const std::string& defaultValue = "")
{
    const std::string* pValue = std::get_if<std::string>(&value);
    if (!pValue)
    {
        BMCWEB_LOG_ERROR
            << "Unable to get value, no std::string data in variant, "
               "hence using default value";
        src[key] = defaultValue;
        return false;
    }

    src[key] = *pValue;

    return true;
}

/**
 * populateResponseOptionsTable adds options array, in a BIOS knob,
 * into the user provided json.
 */
bool populateResponseOptionsTable(
    const std::vector<OptionsItemType>& optionsVector,
    nlohmann::json& optionsArray)
{
    for (const OptionsItemType& optItem : optionsVector)
    {
        nlohmann::json optItemJson;

        const std::string& strOptItemType = std::get<bios::Type>(optItem);
        if ("xyz.openbmc_project.BIOSConfig.Manager.BoundType.OneOf" ==
            std::get<bios::Type>(optItem))
        {
            if (!bios::populateJsonTypeString(optItemJson, "OneOf",
                                              std::get<bios::Value>(optItem)))
            {
                continue;
            }
        }
        else
        {
            BMCWEB_LOG_ERROR << "Unknown option item type == "
                             << strOptItemType;
            continue;
        }

        optionsArray.push_back(optItemJson);
    }

    if (optionsArray.empty())
    {
        BMCWEB_LOG_ERROR << "Options table is empty";
        return false;
    }

    return true;
}

/**
 * populateResponseBiosTableItem adds one BIOS knob into
 * attribute array json.
 */
bool populateResponseBiosTableItem(const bios::BaseTableItemType& item,
                                   nlohmann::json& attributeArray)
{
    const std::string& itemType = std::get<bios::AttrType>(item.second);

    nlohmann::json attributeItem;
    attributeItem["AttributeName"] = item.first;
    attributeItem["ReadOnly"] = std::get<bios::ReadonlyStatus>(item.second);
    attributeItem["DisplayName"] = std::get<bios::DisplayName>(item.second);
    attributeItem["HelpText"] = std::get<bios::Description>(item.second);
    attributeItem["MenuPath"] = std::get<bios::MenuPath>(item.second);

    if (itemType ==
        "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.String")
    {
        attributeItem["Type"] = "String";
        if (!bios::populateJsonTypeString(
                attributeItem, "CurrentValue",
                std::get<bios::CurrValue>(item.second)))
        {
            return false;
        }

        if (!bios::populateJsonTypeString(
                attributeItem, "DefaultValue",
                std::get<bios::DefaultValue>(item.second)))
        {
            return false;
        }
    }
    else if (itemType ==
             "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Integer")
    {
        attributeItem["Type"] = "Integer";
        if (!bios::populateJsonTypeInt64(
                attributeItem, "CurrentValue",
                std::get<bios::CurrValue>(item.second)))
        {
            return false;
        }

        if (!bios::populateJsonTypeInt64(
                attributeItem, "DefaultValue",
                std::get<bios::DefaultValue>(item.second)))
        {
            return false;
        }
    }
    else
    {
        BMCWEB_LOG_ERROR << "Unknown BIOS attribute type == " << itemType;
        return false;
    }

    nlohmann::json optionsArray = nlohmann::json::array();
    const std::vector<OptionsItemType>& optionsVector =
        std::get<bios::Options>(item.second);

    /* Populate options array to response */
    if (!bios::populateResponseOptionsTable(optionsVector, optionsArray))
    {
        return false;
    }

    attributeItem["Value"] = optionsArray;
    attributeArray.push_back(attributeItem);

    return true;
}
} // namespace bios

/**
 * BiosService class supports handle get method for bios.
 */
inline void
    handleBiosServiceGet(const crow::Request&,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Systems/system/Bios";
    asyncResp->res.jsonValue["@odata.type"] = "#Bios.v1_1_0.Bios";
    asyncResp->res.jsonValue["Name"] = "BIOS Configuration";
    asyncResp->res.jsonValue["Description"] = "BIOS Configuration Service";
    asyncResp->res.jsonValue["Id"] = "BIOS";
    asyncResp->res.jsonValue["Actions"]["#Bios.ResetBios"] = {
        {"target", "/redfish/v1/Systems/system/Bios/Actions/Bios.ResetBios"}};

    // Get the ActiveSoftwareImage and SoftwareImages
    fw_util::populateFirmwareInformation(asyncResp, fw_util::biosPurpose, "",
                                         true);
}
inline void requestRoutesBiosService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/Bios/")
        .privileges(redfish::privileges::getBios)
        .methods(boost::beast::http::verb::get)(handleBiosServiceGet);
}

/**
 * BiosReset class supports handle POST method for Reset bios.
 * The class retrieves and sends data directly to D-Bus.
 *
 * Function handles POST method request.
 * Analyzes POST body message before sends Reset request data to D-Bus.
 */
inline void
    handleBiosResetPost(const crow::Request&,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
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
        .methods(boost::beast::http::verb::post)(handleBiosResetPost);
}

/**
 * handleBiosAttributeRegistryGet handle GET method for
 * BiosAttributeRegistry. BiosAttributeRegistry is the list of BIOS
 * nodes/attributes. The function retrieves data directly from D-Bus.
 */
inline void handleBiosAttributeRegistryGet(
    const crow::Request&, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
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
                    const GetObjectType& getObjectType) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "ObjectMapper::GetObject call failed: "
                                 << ec;
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

            /* Get bios base table using dbus calls */
            crow::connections::systemBus->async_method_call(
                [asyncResp](
                    const boost::system::error_code ec,
                    const std::variant<bios::BaseTableType>& retBiosTable) {
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR
                            << "getBiosAttributeRegistry DBUS error: " << ec;
                        messages::resourceNotFound(asyncResp->res,
                                                   "Registries/Bios", "Bios");
                        return;
                    }
                    const bios::BaseTableType* baseBiosTable =
                        std::get_if<bios::BaseTableType>(&retBiosTable);
                    nlohmann::json& attributeArray =
                        asyncResp->res
                            .jsonValue["RegistryEntries"]["Attributes"];
                    if (baseBiosTable == nullptr)
                    {
                        BMCWEB_LOG_ERROR << "baseBiosTable == nullptr ";
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    /* Populate BIOS knobs one by one into response */
                    for (const bios::BaseTableItemType& item : *baseBiosTable)
                    {
                        bios::populateResponseBiosTableItem(item,
                                                            attributeArray);
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

inline void requestRoutesBiosAttributeService(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Registries/BiosAttributeRegistry/BiosAttributeRegistry/")
        .privileges(redfish::privileges::getBios)
        .methods(boost::beast::http::verb::get)(handleBiosAttributeRegistryGet);
}
} // namespace redfish
