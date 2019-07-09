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
#include "registries/openbmc_message_registry.hpp"

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
        // Collections don't include the static data added by SubRoute because
        // it has a duplicate entry for members

        res.jsonValue = {
            {"@odata.type",
             "#MessageRegistryFileCollection.MessageRegistryFileCollection"},
            {"@odata.context", "/redfish/v1/"
                               "$metadata#MessageRegistryFileCollection."
                               "MessageRegistryFileCollection"},
            {"@odata.id", "/redfish/v1/Registries"},
            {"Name", "MessageRegistryFile Collection"},
            {"Description", "Collection of MessageRegistryFiles"},
            {"Members@odata.count", 2},
            {"Members",
             {{{"@odata.id", "/redfish/v1/Registries/Base"}},
              {{"@odata.id", "/redfish/v1/Registries/OpenBMC"}}}}};

        res.end();
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
        res.jsonValue = {
            {"@odata.id", "/redfish/v1/Registries/Base"},
            {"@odata.type", "#MessageRegistryFile.v1_1_0.MessageRegistryFile"},
            {"@odata.context",
             "/redfish/v1/$metadata#MessageRegistryFile.MessageRegistryFile"},
            {"Name", "Base Message Registry File"},
            {"Description", "DMTF Base Message Registry File Location"},
            {"Id", message_registries::base::header.registryPrefix},
            {"Registry", message_registries::base::header.id},
            {"Languages", {"en"}},
            {"Languages@odata.count", 1},
            {"Location",
             {{{"Language", "en"},
               {"PublicationUri",
                "https://redfish.dmtf.org/registries/Base.1.4.0.json"},
               {"Uri", "/redfish/v1/Registries/Base/Base"}}}},
            {"Location@odata.count", 1}};
        res.end();
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
        res.jsonValue = {
            {"@Redfish.Copyright", message_registries::base::header.copyright},
            {"@odata.type", message_registries::base::header.type},
            {"Id", message_registries::base::header.id},
            {"Name", message_registries::base::header.name},
            {"Language", message_registries::base::header.language},
            {"Description", message_registries::base::header.description},
            {"RegistryPrefix", message_registries::base::header.registryPrefix},
            {"RegistryVersion",
             message_registries::base::header.registryVersion},
            {"OwningEntity", message_registries::base::header.owningEntity}};

        nlohmann::json &messageObj = res.jsonValue["Messages"];

        // Go through the Message Registry and populate each Message
        for (const message_registries::MessageEntry &message :
             message_registries::base::registry)
        {
            nlohmann::json &obj = messageObj[message.first];
            obj = {{"Description", message.second.description},
                   {"Message", message.second.message},
                   {"Severity", message.second.severity},
                   {"NumberOfArgs", message.second.numberOfArgs},
                   {"Resolution", message.second.resolution}};
            if (message.second.numberOfArgs > 0)
            {
                nlohmann::json &messageParamArray = obj["ParamTypes"];
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
        res.end();
    }
};

class OpenBMCMessageRegistryFile : public Node
{
  public:
    template <typename CrowApp>
    OpenBMCMessageRegistryFile(CrowApp &app) :
        Node(app, "/redfish/v1/Registries/OpenBMC/")
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
        res.jsonValue = {
            {"@odata.id", "/redfish/v1/Registries/OpenBMC"},
            {"@odata.type", "#MessageRegistryFile.v1_1_0.MessageRegistryFile"},
            {"@odata.context",
             "/redfish/v1/$metadata#MessageRegistryFile.MessageRegistryFile"},
            {"Name", "Open BMC Message Registry File"},
            {"Description", "Open BMC Message Registry File Location"},
            {"Id", message_registries::openbmc::header.registryPrefix},
            {"Registry", message_registries::openbmc::header.id},
            {"Languages", {"en"}},
            {"Languages@odata.count", 1},
            {"Location",
             {{{"Language", "en"},
               {"Uri", "/redfish/v1/Registries/OpenBMC/OpenBMC"}}}},
            {"Location@odata.count", 1}};

        res.end();
    }
};

class OpenBMCMessageRegistry : public Node
{
  public:
    template <typename CrowApp>
    OpenBMCMessageRegistry(CrowApp &app) :
        Node(app, "/redfish/v1/Registries/OpenBMC/OpenBMC/")
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
        res.jsonValue = {
            {"@Redfish.Copyright",
             message_registries::openbmc::header.copyright},
            {"@odata.type", message_registries::openbmc::header.type},
            {"Id", message_registries::openbmc::header.id},
            {"Name", message_registries::openbmc::header.name},
            {"Language", message_registries::openbmc::header.language},
            {"Description", message_registries::openbmc::header.description},
            {"RegistryPrefix",
             message_registries::openbmc::header.registryPrefix},
            {"RegistryVersion",
             message_registries::openbmc::header.registryVersion},
            {"OwningEntity", message_registries::openbmc::header.owningEntity}};

        nlohmann::json &messageObj = res.jsonValue["Messages"];
        // Go through the Message Registry and populate each Message
        for (const message_registries::MessageEntry &message :
             message_registries::openbmc::registry)
        {
            nlohmann::json &obj = messageObj[message.first];
            obj = {{"Description", message.second.description},
                   {"Message", message.second.message},
                   {"Severity", message.second.severity},
                   {"NumberOfArgs", message.second.numberOfArgs},
                   {"Resolution", message.second.resolution}};
            if (message.second.numberOfArgs > 0)
            {
                nlohmann::json &messageParamArray = obj["ParamTypes"];
                for (size_t i = 0; i < message.second.numberOfArgs; i++)
                {
                    messageParamArray.push_back(message.second.paramTypes[i]);
                }
            }
        }
        res.end();
    }
};

} // namespace redfish
