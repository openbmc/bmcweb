#pragma once

#include "node.hpp"

#include <tinyxml2.h>

namespace redfish
{
/**
 * BiosService class supports handle get method for bios.
 */
class BiosService : public Node
{
  public:
    BiosService(CrowApp &app) : Node(app, "/redfish/v1/Systems/system/Bios/")
    {
        entityPrivileges = {{boost::beast::http::verb::get, {{"Login"}}}};
    }

  private:
    static constexpr const char *biosAttributeFilePath =
        "/tmp/BIOSConfig/BiosAttribute.json";
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
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
        asyncResp->res.jsonValue["Attributes"] = nlohmann::json::object();
        getBiosAttributes(asyncResp);
    }

    void getBiosAttributes(std::shared_ptr<AsyncResp> asyncResp)
    {
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG << "getBiosAttributes DBUS error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                nlohmann::json &attributeArray =
                    asyncResp->res.jsonValue["Attributes"];
                std::ifstream file(biosAttributeFilePath);
                attributeArray = nlohmann::json::parse(file);
            },
            "xyz.openbmc_project.biosconfig_manager",
            "/xyz/openbmc_project/BIOSConfig/BIOSConfigMgr",
            "xyz.openbmc_project.BIOSConfig.BIOSConfigMgr", "GetAllAttributes",
            static_cast<uint8_t>(0));
    }
};
/**
 * BiosReset class supports handle POST method for Reset bios.
 * The class retrieves and sends data directly to D-Bus.
 */
class BiosReset : public Node
{
  public:
    BiosReset(CrowApp &app) :
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
    void doPost(crow::Response &res, const crow::Request &req,
                const std::vector<std::string> &params) override
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
/**
 * BiosAttributeRegistry class supports handle get method for BIOS attribute
 * registry.
 */
class BiosAttributeRegistry : public Node
{
  public:
    static constexpr const char *biosAttributeRegistryFilePath =
        "/tmp/BIOSConfig/BiosAttributeRegistry.json";
    BiosAttributeRegistry(CrowApp &app) :
        Node(app, "/redfish/v1/Registries/Bios/")
    {
        entityPrivileges = {{boost::beast::http::verb::get, {{"Login"}}}};
    }

  private:
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Registries/Bios";
        asyncResp->res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#AttributeRegistry.AttributeRegistry";
        asyncResp->res.jsonValue["@odata.type"] =
            "#AttributeRegistry.v1_3_2.AttributeRegistry";
        asyncResp->res.jsonValue["Name"] = "Bios Attribute Registry Version 1";
        asyncResp->res.jsonValue["Id"] = "BiosAttributeRegistryV1";
        asyncResp->res.jsonValue["RegistryVersion"] = "1.0.0";
        asyncResp->res.jsonValue["Language"] = "en";
        asyncResp->res.jsonValue["OwningEntity"] = "Intel";
        asyncResp->res.jsonValue["SupportedSystems"] = nlohmann::json::array();

        getBiosAttributeRegistry(asyncResp);
    }

    /**
     * Get BIOS attribute registry from d-bus.
     */
    void getBiosAttributeRegistry(std::shared_ptr<AsyncResp> asyncResp)
    {
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG << "getBiosAttributeRegistry DBUS error: "
                                     << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                nlohmann::json &attributeArray =
                    asyncResp->res.jsonValue["RegistryEntries"];
                std::ifstream file(biosAttributeRegistryFilePath);
                attributeArray = nlohmann::json::parse(file);
            },
            "xyz.openbmc_project.biosconfig_manager",
            "/xyz/openbmc_project/BIOSConfig/BIOSConfigMgr",
            "xyz.openbmc_project.BIOSConfig.BIOSConfigMgr", "GetAllAttributes",
            static_cast<uint8_t>(1));
    }
};
} // namespace redfish
