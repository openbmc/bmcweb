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
+        asyncResp->res.jsonValue["AttributeRegistry"] =
+            "BiosAttributeRegistryV1_0_0";
+        asyncResp->res.jsonValue["Attributes"] = nlohmann::json::object();
+        getBiosAttributes(asyncResp);
+        generateBiosAttributeRegistry();
+    }
+
+    void getBiosAttributes(std::shared_ptr<AsyncResp> asyncResp)
+    {
+        nlohmann::json &jResp = asyncResp->res.jsonValue["Attributes"];
+        tinyxml2::XMLDocument xmlDoc;
+        xmlDoc.LoadFile("/tmp/bios.xml");
+        tinyxml2::XMLNode *pRoot = xmlDoc.FirstChild();
+        if (pRoot == nullptr)
+            return;
+        tinyxml2::XMLElement *pElement = pRoot->FirstChildElement("biosknobs");
+        if (pElement == nullptr)
+            return;
+        tinyxml2::XMLElement *pKnobsElement =
+            pElement->FirstChildElement("knob");
+        while (pKnobsElement != nullptr)
+        {
+            std::string name = pKnobsElement->Attribute("name");
+            std::string value = pKnobsElement->Attribute("CurrentVal");
+            jResp.push_back({name, value});
+            pKnobsElement = pKnobsElement->NextSiblingElement("knob");
+        }
+
+        return;
+    }
+
+    void generateBiosAttributeRegistry()
+    {
+        nlohmann::json biosAttributeRegistry;
+        biosAttributeRegistry["@odata.context"] =
+            "/redfish/v1/$metadata#AttributeRegistry.AttributeRegistry";
+        biosAttributeRegistry["@odata.type"] =
+            "#AttributeRegistry.v1_1_0.AttributeRegistry";
+        biosAttributeRegistry["Name"] = "Bios Attribute Registry Version 1";
+        biosAttributeRegistry["OwningEntity"] = "Intel";
+        biosAttributeRegistry["Id"] = "BiosAttributeRegistryV1_0_0";
+        biosAttributeRegistry["RegistryVersion"] = "1.0.0";
+        biosAttributeRegistry["Language"] = "en";
+
+        tinyxml2::XMLDocument xmlDoc;
+        xmlDoc.LoadFile("/tmp/bios.xml");
+        tinyxml2::XMLNode *pRoot = xmlDoc.FirstChild();
+        if (pRoot == nullptr)
+            return;
+        tinyxml2::XMLElement *pElement = pRoot->FirstChildElement("biosknobs");
+        if (pElement == nullptr)
+            return;
+        tinyxml2::XMLElement *pKnobsElement =
+            pElement->FirstChildElement("knob");
+
+        nlohmann::json &attributeArray =
+            biosAttributeRegistry["RegistryEntries"]["Attributes"];
+        attributeArray = nlohmann::json::array();
+        while (pKnobsElement != nullptr)
+        {
+            const char *name = pKnobsElement->Attribute("name");
+            const char *value = pKnobsElement->Attribute("CurrentVal");
+            const char *dname = pKnobsElement->Attribute("prompt");
+            const char *type = pKnobsElement->Attribute("type");
+            const char *setuptype = pKnobsElement->Attribute("setupType");
+            const char *disc = pKnobsElement->Attribute("CurrentVal");
+            const char *menupath = pKnobsElement->Attribute("SetupPgPtr");
+            const char *size = pKnobsElement->Attribute("size");
+            const char *offset = pKnobsElement->Attribute("offset");
+            const char *defaultv = pKnobsElement->Attribute("default");
+
+            if (name != nullptr && value != nullptr && dname != nullptr &&
+                type != nullptr && setuptype != nullptr && disc != nullptr &&
+                menupath != nullptr && size != nullptr && offset != nullptr &&
+                defaultv != nullptr)
+            {
+                tinyxml2::XMLElement *pOptionsElement =
+                    pKnobsElement->FirstChildElement("options");
+                nlohmann::json optionsArray = nlohmann::json::array();
+                if (pOptionsElement != nullptr)
+                {
+                    tinyxml2::XMLElement *pOptionElement =
+                        pOptionsElement->FirstChildElement("option");
+                    while (pOptionElement != nullptr)
+                    {
+                        const char *text = pOptionElement->Attribute("text");
+                        const char *value = pOptionElement->Attribute("value");
+                        if (text != nullptr && value != nullptr)
+                        {
+                            optionsArray.push_back({text, value});
+                        }
+                        pOptionElement =
+                            pOptionElement->NextSiblingElement("option");
+                    }
+                }
+
+                attributeArray.push_back({{"AttributeName", name},
+                                          {"CurrentValue", value},
+                                          {"DisplayName", dname},
+                                          {"Type", type},
+                                          {"SetupType", setuptype},
+                                          {"Discription", disc},
+                                          {"MenuPath", menupath},
+                                          {"Size", size},
+                                          {"Offset", offset},
+                                          {"DefaultValue", defaultv},
+                                          {"Options", optionsArray}});
+            }
+            pKnobsElement = pKnobsElement->NextSiblingElement("knob");
+        }
+
+        std::ofstream file("/tmp/BiosAttributeRegistry.json");
+        file << biosAttributeRegistry.dump(4) << std::endl;
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
} // namespace redfish
