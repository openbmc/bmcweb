#pragma once

#include "node.hpp"

#include <tinyxml2.h>

#include <fstream>

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
        setAttributeRegistryToDbus(asyncResp);
        getAttributeRegistryFromDbus(asyncResp);
    }

    /**
     * Get BIOS attribute registry from d-bus.
     */
    void getAttributeRegistryFromDbus(std::shared_ptr<AsyncResp> asyncResp)
    {
        asyncResp->res.jsonValue["RegistryEntriesFromDbus"] = "test";

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const std::variant<std::string> &attributeString) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG
                        << "getAttributeRegistryFromDbus DBUS error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
                nlohmann::json &attributeArray =
                    asyncResp->res.jsonValue["RegistryEntriesFromDbus"];
                // attributeArray = nlohmann::json::object();
                // std::stringstream ss;
                // ss << attributeString.str();
                const std::string *attrStr =
                    std::get_if<std::string>(&attributeString);
                if (attrStr != nullptr)
                {
                    attributeArray = nlohmann::json::parse(attrStr->c_str());
                }
            },
            "xyz.openbmc_project.Settings",
            "/xyz/openbmc_project/software/bios",
            "org.freedesktop.DBus.Properties", "Get",
            "xyz.openbmc_project.Software.Version", "Version");
    }

    /**
     * Set BIOS attribute registry to d-bus.
     */
    void setAttributeRegistryToDbus(std::shared_ptr<AsyncResp> asyncResp)
    {
        nlohmann::json &attributeArray =
            asyncResp->res.jsonValue["RegistryEntries"];
        std::string registryEntriesStr = attributeArray.dump();

        crow::connections::systemBus->async_method_call(
            [registryEntriesStr,
             asyncResp](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG
                        << "setAttributeRegistryToDbus DBUS error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
            },
            "xyz.openbmc_project.Settings",
            "/xyz/openbmc_project/software/bios",
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Software.Version", "Version",
            sdbusplus::message::variant<std::string>(registryEntriesStr));
    }
    /**
     * Generate BIOS attribute registry form bios.xml file.
     */
    void getBiosAttributeRegistry(std::shared_ptr<AsyncResp> asyncResp)
    {
        tinyxml2::XMLDocument xmlDoc;
        xmlDoc.LoadFile("/tmp/bios.xml");
        tinyxml2::XMLNode *pRoot = xmlDoc.FirstChild();
        if (pRoot == nullptr)
            return;
        tinyxml2::XMLElement *pElement = pRoot->FirstChildElement("biosknobs");
        if (pElement == nullptr)
            return;
        tinyxml2::XMLElement *pKnobsElement =
            pElement->FirstChildElement("knob");

        nlohmann::json &attributeArray =
            asyncResp->res.jsonValue["RegistryEntries"]["Attributes"];
        attributeArray = nlohmann::json::array();
        while (pKnobsElement != nullptr)
        {
            const char *name = pKnobsElement->Attribute("name");
            const char *value = pKnobsElement->Attribute("CurrentVal");
            const char *dname = pKnobsElement->Attribute("prompt");
            const char *type = pKnobsElement->Attribute("type");
            const char *setuptype = pKnobsElement->Attribute("setupType");
            const char *disc = pKnobsElement->Attribute("CurrentVal");
            const char *menupath = pKnobsElement->Attribute("SetupPgPtr");
            const char *size = pKnobsElement->Attribute("size");
            const char *offset = pKnobsElement->Attribute("offset");
            const char *defaultv = pKnobsElement->Attribute("default");
            const char *dorder = pKnobsElement->Attribute("varstoreIndex");

            if (name != nullptr && value != nullptr && dname != nullptr &&
                type != nullptr && setuptype != nullptr && disc != nullptr &&
                menupath != nullptr && size != nullptr && offset != nullptr &&
                defaultv != nullptr && dorder != nullptr)
            {
                tinyxml2::XMLElement *pOptionsElement =
                    pKnobsElement->FirstChildElement("options");
                nlohmann::json optionsArray = nlohmann::json::array();
                if (pOptionsElement != nullptr)
                {
                    tinyxml2::XMLElement *pOptionElement =
                        pOptionsElement->FirstChildElement("option");
                    while (pOptionElement != nullptr)
                    {
                        const char *text = pOptionElement->Attribute("text");
                        const char *value = pOptionElement->Attribute("value");
                        if (text != nullptr && value != nullptr)
                        {
                            optionsArray.push_back({{"ValueDisplayName", text},
                                                    {"ValueName", value}});
                        }
                        pOptionElement =
                            pOptionElement->NextSiblingElement("option");
                    }
                }

                attributeArray.push_back({{"AttributeName", name},
                                          {"CurrentValue", value},
                                          {"DisplayName", dname},
                                          {"Type", type},
                                          {"SetupType", setuptype},
                                          {"Discription", disc},
                                          {"MenuPath", menupath},
                                          {"Size", size},
                                          {"Offset", offset},
                                          {"DefaultValue", defaultv},
                                          {"DisplayOrder", dorder},
                                          {"Value", optionsArray}});
            }
            pKnobsElement = pKnobsElement->NextSiblingElement("knob");
        }
    }
};
} // namespace redfish
