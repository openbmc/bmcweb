#pragma once

#include <app.hpp>
#include <query.hpp>
#include <registries/privilege_registry.hpp>
#include <utils/sw_utils.hpp>

namespace redfish
{
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

    asyncResp->res.jsonValue["Actions"]["#Bios.ChangePassword"] = {
        {"target", "/redfish/v1/Systems/system/Bios/Actions/"
                   "Bios.ChangePassword"}};

    // Get the ActiveSoftwareImage and SoftwareImages
    fw_util::populateFirmwareInformation(asyncResp, fw_util::biosPurpose, "",
                                         true);

    asyncResp->res.jsonValue["@Redfish.Settings"] = {
        {"@odata.type", "#Settings.v1_3_0.Settings"},
        {"SettingsObject",
         {{"@odata.id", "/redfish/v1/Systems/system/Bios/Settings"}}}};
    asyncResp->res.jsonValue["AttributeRegistry"] = "BiosAttributeRegistry";
    asyncResp->res.jsonValue["Attributes"] = nlohmann::json::object();

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

        const std::string& service = getObjectType.begin()->first;

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const std::variant<BiosBaseTableType>& retBiosTable) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "getBiosAttributes DBUS error: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }
            const BiosBaseTableType* baseBiosTable =
                std::get_if<BiosBaseTableType>(&retBiosTable);
            nlohmann::json& attributesJson =
                asyncResp->res.jsonValue["Attributes"];
            if (baseBiosTable == nullptr)
            {
                BMCWEB_LOG_ERROR << "baseBiosTable is empty";
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
                    BMCWEB_LOG_ERROR << "Unsupported attribute type.";
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

} // namespace redfish
