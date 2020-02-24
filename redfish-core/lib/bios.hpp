#pragma once

#include "node.hpp"

#include <tinyxml2.h>

namespace redfish
{
enum class AttributesModel
{
    Attributeslist = 0,
    AttributesRegistry = 1,
    PendingAttributelist = 2
};
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
                if (!std::filesystem::exists(biosAttributeFilePath))
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                std::ifstream file(biosAttributeFilePath);
                attributeArray = nlohmann::json::parse(file);
                file.close();
                std::remove(biosAttributeFilePath);
            },
            "xyz.openbmc_project.biosconfig_manager",
            "/xyz/openbmc_project/BIOSConfig/BIOSConfigMgr",
            "xyz.openbmc_project.BIOSConfig.BIOSConfigMgr", "GetAllAttributes",
            static_cast<uint8_t>(AttributesModel::Attributeslist));
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
 * BiosSettings class supports handle GET/PATCH method for
 * BIOS configuration pending settings.
 */
class BiosSettings : public Node
{
  public:
    static constexpr const char *biosSettingsFilePath =
        "/tmp/BIOSConfig/BiosSettings.json";
    BiosSettings(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/system/Bios/Settings")
    {
        entityPrivileges = {{boost::beast::http::verb::get, {{"Login"}}}};
    }

  private:
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/Bios/Settings";
        asyncResp->res.jsonValue["@odata.type"] = "#Bios.v1_1_0.Bios";
        asyncResp->res.jsonValue["Name"] = "Bios Settings Version 1";
        asyncResp->res.jsonValue["Id"] = "BiosSettingsV1";
        asyncResp->res.jsonValue["AttributeRegistry"] =
            "BiosAttributeRegistryV1";
        asyncResp->res.jsonValue["Attributes"] = nlohmann::json::object();

        getBiosSettings(asyncResp);
    }
    void doPatch(crow::Response &res, const crow::Request &req,
                 const std::vector<std::string> &params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        nlohmann::json jsonRequest;
        if (!json_util::processJsonFromRequest(res, req, jsonRequest))
        {
            BMCWEB_LOG_ERROR << "processJsonFromRequest error";
            messages::internalError(res);
            return;
        }
        if (std::filesystem::exists(biosSettingsFilePath))
        {
            nlohmann::json biosSettingsJson;
            std::ifstream ifile;
            ifile.open(biosSettingsFilePath);
            biosSettingsJson = nlohmann::json::parse(ifile);
            biosSettingsJson.merge_patch(jsonRequest);
            ifile.close();

            std::ofstream ofile;
            ofile.open(biosSettingsFilePath);
            ofile << biosSettingsJson.dump() << std::endl;
            ofile.close();
        }
        else
        {
            std::ofstream ofile;
            ofile.open(biosSettingsFilePath);
            ofile << jsonRequest.dump() << std::endl;
            ofile.close();
        }

        patchBiosSettings(asyncResp);
    }
    /**
     * Patch BIOS Settings from d-bus.
     */
    void patchBiosSettings(std::shared_ptr<AsyncResp> asyncResp)
    {
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "patchBiosSettings DBUS error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                messages::success(asyncResp->res);
            },
            "xyz.openbmc_project.biosconfig_manager",
            "/xyz/openbmc_project/BIOSConfig/BIOSConfigMgr",
            "xyz.openbmc_project.BIOSConfig.BIOSConfigMgr",
            "PatchBiosSettings");
    }
    /**
     * Get BIOS Settings from d-bus.
     */
    void getBiosSettings(std::shared_ptr<AsyncResp> asyncResp)
    {
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "getBiosSettings DBUS error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                nlohmann::json &attributeArray =
                    asyncResp->res.jsonValue["Attributes"];
                if (!std::filesystem::exists(biosSettingsFilePath))
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                std::ifstream file(biosSettingsFilePath);
                attributeArray = nlohmann::json::parse(file);
                file.close();
            },
            "xyz.openbmc_project.biosconfig_manager",
            "/xyz/openbmc_project/BIOSConfig/BIOSConfigMgr",
            "xyz.openbmc_project.BIOSConfig.BIOSConfigMgr", "GetAllAttributes",
            static_cast<uint8_t>(AttributesModel::PendingAttributelist));
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
                if (!std::filesystem::exists(biosAttributeRegistryFilePath))
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                std::ifstream file(biosAttributeRegistryFilePath);
                attributeArray = nlohmann::json::parse(file);
                file.close();
                std::remove(biosAttributeRegistryFilePath);
            },
            "xyz.openbmc_project.biosconfig_manager",
            "/xyz/openbmc_project/BIOSConfig/BIOSConfigMgr",
            "xyz.openbmc_project.BIOSConfig.BIOSConfigMgr", "GetAllAttributes",
            static_cast<uint8_t>(AttributesModel::AttributesRegistry));
    }
};
} // namespace redfish
