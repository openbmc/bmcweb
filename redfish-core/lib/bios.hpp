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

        nlohmann::json& jResp = asyncResp->res.jsonValue["Attributes"];
        tinyxml2::XMLDocument xmlDoc;
        xmlDoc.LoadFile("/tmp/bios.xml");
        tinyxml2::XMLNode *pRoot = xmlDoc.FirstChild();
        if (pRoot == nullptr) return;
        tinyxml2::XMLElement *pElement = pRoot->FirstChildElement("biosknobs");
        if (pElement == nullptr) return;
        tinyxml2::XMLElement *pKnobsElement = pElement->FirstChildElement("knob");
        while (pKnobsElement != nullptr)
        {
            std::string name = pKnobsElement->Attribute("name");
            std::string value = pKnobsElement->Attribute("CurrentVal");
            jResp.push_back({name, value});
            pKnobsElement = pKnobsElement->NextSiblingElement("knob");
        }

        return;
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
