/*
// Copyright (c) 2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#pragma once

#include "node.hpp"
#include "registries.hpp"
#include "registries/base_message_registry.hpp"
#include "registries/resource_event_message_registry.hpp"

namespace redfish
{

class MessageRegistryFileCollection : public Node
{
  public:
    template <typename CrowApp>
    MessageRegistryFileCollection(CrowApp &app) :
        Node(app, "/redfish/v1/Registries/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    /**
     * Functions triggers appropriate requests on DBus
     */
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        // Collections don't include the static data added by SubRoute because
        // it has a duplicate entry for members
        asyncResp->res.jsonValue["@odata.type"] =
            "#MessageRegistryFileCollection.MessageRegistryFileCollection";
        asyncResp->res.jsonValue["@odata.context"] =
            "/redfish/v1/"
            "$metadata#MessageRegistryFileCollection."
            "MessageRegistryFileCollection";
        asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Registries";
        asyncResp->res.jsonValue["Name"] = "MessageRegistryFile Collection";
        asyncResp->res.jsonValue["Description"] =
            "Collection of MessageRegistryFiles";
        nlohmann::json &messageRegistryFileArray =
            asyncResp->res.jsonValue["Members"];
        messageRegistryFileArray = nlohmann::json::array();
        messageRegistryFileArray.push_back(
            {{"@odata.id", "/redfish/v1/Registries/Base"}});
        messageRegistryFileArray.push_back(
            {{"@odata.id", "/redfish/v1/Registries/ResourceEvent"}});
        messageRegistryFileArray.push_back(
            {{"@odata.id", "/redfish/v1/Registries/OpenBMC"}});
        asyncResp->res.jsonValue["Members@odata.count"] =
            messageRegistryFileArray.size();
    }
};

class BaseMessageRegistryFile : public Node
{
  public:
    template <typename CrowApp>
    BaseMessageRegistryFile(CrowApp &app) :
        Node(app, "/redfish/v1/Registries/Base/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

        asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Registries/Base";
        asyncResp->res.jsonValue["@odata.type"] =
            "#MessageRegistryFile.v1_1_0.MessageRegistryFile";
        asyncResp->res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#MessageRegistryFile.MessageRegistryFile";
        asyncResp->res.jsonValue["Name"] = "Base Message Registry File";
        asyncResp->res.jsonValue["Description"] =
            "DMTF Base Message Registry File Location";
        asyncResp->res.jsonValue["Id"] =
            message_registries::base::header.registryPrefix;
        asyncResp->res.jsonValue["Registry"] =
            message_registries::base::header.id;
        nlohmann::json &messageRegistryLanguageArray =
            asyncResp->res.jsonValue["Languages"];
        messageRegistryLanguageArray = nlohmann::json::array();
        messageRegistryLanguageArray.push_back({"en"});
        asyncResp->res.jsonValue["Languages@odata.count"] =
            messageRegistryLanguageArray.size();
        nlohmann::json &messageRegistryLocationArray =
            asyncResp->res.jsonValue["Location"];
        messageRegistryLocationArray = nlohmann::json::array();
        messageRegistryLocationArray.push_back(
            {{"Language", "en"},
             {"PublicationUri",
              "https://redfish.dmtf.org/registries/Base.1.4.0.json"},
             {"Uri", "/redfish/v1/Registries/Base/Base"}});
        asyncResp->res.jsonValue["Location@odata.count"] =
            messageRegistryLocationArray.size();
    }
};

class BaseMessageRegistry : public Node
{
  public:
    template <typename CrowApp>
    BaseMessageRegistry(CrowApp &app) :
        Node(app, "/redfish/v1/Registries/Base/Base/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

        asyncResp->res.jsonValue["@Redfish.Copyright"] =
            message_registries::base::header.copyright;
        asyncResp->res.jsonValue["@odata.type"] =
            message_registries::base::header.type;
        asyncResp->res.jsonValue["Id"] = message_registries::base::header.id;
        asyncResp->res.jsonValue["Name"] =
            message_registries::base::header.name;
        asyncResp->res.jsonValue["Language"] =
            message_registries::base::header.language;
        asyncResp->res.jsonValue["Description"] =
            message_registries::base::header.description;
        asyncResp->res.jsonValue["RegistryPrefix"] =
            message_registries::base::header.registryPrefix;
        asyncResp->res.jsonValue["RegistryVersion"] =
            message_registries::base::header.registryVersion;
        asyncResp->res.jsonValue["OwningEntity"] =
            message_registries::base::header.owningEntity;
        nlohmann::json &messageArray = asyncResp->res.jsonValue["Messages"];
        messageArray = nlohmann::json::array();

        // Go through the Message Registry and populate each Message
        for (const message_registries::MessageEntry &message :
             message_registries::base::registry)
        {
            messageArray.push_back(
                {{message.first,
                  {{"Description", message.second.description},
                   {"Message", message.second.message},
                   {"Severity", message.second.severity},
                   {"NumberOfArgs", message.second.numberOfArgs},
                   {"Resolution", message.second.resolution}}}});
            if (message.second.numberOfArgs > 0)
            {
                nlohmann::json &messageParamArray =
                    messageArray.back()[message.first]["ParamTypes"];
                for (const char *str : message.second.paramTypes)
                {
                    if (str == nullptr)
                    {
                        break;
                    }
                    messageParamArray.push_back(str);
                }
            }
        }
    }
};

class ResourceEventMessageRegistryFile : public Node
{
  public:
    template <typename CrowApp>
    ResourceEventMessageRegistryFile(CrowApp &app) :
        Node(app, "/redfish/v1/Registries/ResourceEvent/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Registries/ResourceEvent";
        asyncResp->res.jsonValue["@odata.type"] =
            "#MessageRegistryFile.v1_1_0.MessageRegistryFile";
        asyncResp->res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#MessageRegistryFile.MessageRegistryFile";
        asyncResp->res.jsonValue["Name"] =
            "Resource Event Message Registry File";
        asyncResp->res.jsonValue["Description"] =
            "DMTF Resource Event Message Registry File Location";
        asyncResp->res.jsonValue["Id"] =
            message_registries::resource_event::header.registryPrefix;
        asyncResp->res.jsonValue["Registry"] =
            message_registries::resource_event::header.id;
        nlohmann::json &messageRegistryLanguageArray =
            asyncResp->res.jsonValue["Languages"];
        messageRegistryLanguageArray = nlohmann::json::array();
        messageRegistryLanguageArray.push_back({"en"});
        asyncResp->res.jsonValue["Languages@odata.count"] =
            messageRegistryLanguageArray.size();
        nlohmann::json &messageRegistryLocationArray =
            asyncResp->res.jsonValue["Location"];
        messageRegistryLocationArray = nlohmann::json::array();
        messageRegistryLocationArray.push_back(
            {{"Language", "en"},
             {"PublicationUri",
              "https://redfish.dmtf.org/registries/ResourceEvent.1.0.0.json"},
             {"Uri", "/redfish/v1/Registries/ResourceEvent/ResourceEvent"}});
        asyncResp->res.jsonValue["Location@odata.count"] =
            messageRegistryLocationArray.size();
    }
};

class ResourceEventMessageRegistry : public Node
{
  public:
    template <typename CrowApp>
    ResourceEventMessageRegistry(CrowApp &app) :
        Node(app, "/redfish/v1/Registries/ResourceEvent/ResourceEvent/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

        asyncResp->res.jsonValue["@Redfish.Copyright"] =
            message_registries::resource_event::header.copyright;
        asyncResp->res.jsonValue["@odata.type"] =
            message_registries::resource_event::header.type;
        asyncResp->res.jsonValue["Id"] =
            message_registries::resource_event::header.id;
        asyncResp->res.jsonValue["Name"] =
            message_registries::resource_event::header.name;
        asyncResp->res.jsonValue["Language"] =
            message_registries::resource_event::header.language;
        asyncResp->res.jsonValue["Description"] =
            message_registries::resource_event::header.description;
        asyncResp->res.jsonValue["RegistryPrefix"] =
            message_registries::resource_event::header.registryPrefix;
        asyncResp->res.jsonValue["RegistryVersion"] =
            message_registries::resource_event::header.registryVersion;
        asyncResp->res.jsonValue["OwningEntity"] =
            message_registries::resource_event::header.owningEntity;
        nlohmann::json &messageArray = asyncResp->res.jsonValue["Messages"];
        messageArray = nlohmann::json::array();

        // Go through the Message Registry and populate each Message
        for (const message_registries::MessageEntry &message :
             message_registries::resource_event::registry)
        {
            messageArray.push_back(
                {{message.first,
                  {{"Description", message.second.description},
                   {"Message", message.second.message},
                   {"Severity", message.second.severity},
                   {"NumberOfArgs", message.second.numberOfArgs},
                   {"Resolution", message.second.resolution}}}});
            if (message.second.numberOfArgs > 0)
            {
                nlohmann::json &messageParamArray =
                    messageArray.back()[message.first]["ParamTypes"];
                for (int i = 0; i < message.second.numberOfArgs; i++)
                {
                    messageParamArray.push_back(message.second.paramTypes[i]);
                }
            }
        }
    }
};

} // namespace redfish
