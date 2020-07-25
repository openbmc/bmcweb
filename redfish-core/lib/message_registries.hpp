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
#include "registries/resource_event_message_registry.hpp"
#include "registries/task_event_message_registry.hpp"

namespace redfish
{

class MessageRegistryFileCollection : public Node
{
  public:
    MessageRegistryFileCollection(App& app) :
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>&) override
    {
        // Collections don't include the static data added by SubRoute because
        // it has a duplicate entry for members

        res.jsonValue = {
            {"@odata.type",
             "#MessageRegistryFileCollection.MessageRegistryFileCollection"},
            {"@odata.id", "/redfish/v1/Registries"},
            {"Name", "MessageRegistryFile Collection"},
            {"Description", "Collection of MessageRegistryFiles"},
            {"Members@odata.count", 4},
            {"Members",
             {{{"@odata.id", "/redfish/v1/Registries/Base"}},
              {{"@odata.id", "/redfish/v1/Registries/TaskEvent"}},
              {{"@odata.id", "/redfish/v1/Registries/ResourceEvent"}},
              {{"@odata.id", "/redfish/v1/Registries/OpenBMC"}}}}};

        res.end();
    }
};

class MessageRegistryFile : public Node
{
  public:
    MessageRegistryFile(App& app) :
        Node(app, "/redfish/v1/Registries/<str>/", std::string())
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        if (params.size() != 1)
        {
            messages::internalError(res);
            res.end();
            return;
        }

        const std::string& registry = params[0];
        const message_registries::Header* header;
        std::string dmtf = "DMTF ";
        const char* url = nullptr;

        if (registry == "Base")
        {
            header = &message_registries::base::header;
            url = message_registries::base::url;
        }
        else if (registry == "TaskEvent")
        {
            header = &message_registries::task_event::header;
            url = message_registries::task_event::url;
        }
        else if (registry == "OpenBMC")
        {
            header = &message_registries::openbmc::header;
            dmtf.clear();
        }
        else if (registry == "ResourceEvent")
        {
            header = &message_registries::resource_event::header;
            url = message_registries::resource_event::url;
        }
        else
        {
            messages::resourceNotFound(
                res, "#MessageRegistryFile.v1_1_0.MessageRegistryFile",
                registry);
            res.end();
            return;
        }

        res.jsonValue = {
            {"@odata.id", "/redfish/v1/Registries/" + registry},
            {"@odata.type", "#MessageRegistryFile.v1_1_0.MessageRegistryFile"},
            {"Name", registry + " Message Registry File"},
            {"Description",
             dmtf + registry + " Message Registry File Location"},
            {"Id", header->registryPrefix},
            {"Registry", header->id},
            {"Languages", {"en"}},
            {"Languages@odata.count", 1},
            {"Location",
             {{{"Language", "en"},
               {"Uri",
                "/redfish/v1/Registries/" + registry + "/" + registry}}}},
            {"Location@odata.count", 1}};

        if (url != nullptr)
        {
            res.jsonValue["Location"][0]["PublicationUri"] = url;
        }

        res.end();
    }
};

class MessageRegistry : public Node
{
  public:
    MessageRegistry(App& app) :
        Node(app, "/redfish/v1/Registries/<str>/<str>/", std::string(),
             std::string())
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        if (params.size() != 2)
        {
            messages::internalError(res);
            res.end();
            return;
        }

        const std::string& registry = params[0];
        const std::string& registry1 = params[1];

        const message_registries::Header* header;
        std::vector<const message_registries::MessageEntry*> registryEntries;
        if (registry == "Base")
        {
            header = &message_registries::base::header;
            for (const message_registries::MessageEntry& entry :
                 message_registries::base::registry)
            {
                registryEntries.emplace_back(&entry);
            }
        }
        else if (registry == "TaskEvent")
        {
            header = &message_registries::task_event::header;
            for (const message_registries::MessageEntry& entry :
                 message_registries::task_event::registry)
            {
                registryEntries.emplace_back(&entry);
            }
        }
        else if (registry == "OpenBMC")
        {
            header = &message_registries::openbmc::header;
            for (const message_registries::MessageEntry& entry :
                 message_registries::openbmc::registry)
            {
                registryEntries.emplace_back(&entry);
            }
        }
        else if (registry == "ResourceEvent")
        {
            header = &message_registries::resource_event::header;
            for (const message_registries::MessageEntry& entry :
                 message_registries::resource_event::registry)
            {
                registryEntries.emplace_back(&entry);
            }
        }
        else
        {
            messages::resourceNotFound(
                res, "#MessageRegistryFile.v1_1_0.MessageRegistryFile",
                registry);
            res.end();
            return;
        }

        if (registry != registry1)
        {
            messages::resourceNotFound(res, header->type, registry1);
            res.end();
            return;
        }

        res.jsonValue = {{"@Redfish.Copyright", header->copyright},
                         {"@odata.type", header->type},
                         {"Id", header->id},
                         {"Name", header->name},
                         {"Language", header->language},
                         {"Description", header->description},
                         {"RegistryPrefix", header->registryPrefix},
                         {"RegistryVersion", header->registryVersion},
                         {"OwningEntity", header->owningEntity}};

        nlohmann::json& messageObj = res.jsonValue["Messages"];

        // Go through the Message Registry and populate each Message
        for (const message_registries::MessageEntry* message : registryEntries)
        {
            nlohmann::json& obj = messageObj[message->first];
            obj = {{"Description", message->second.description},
                   {"Message", message->second.message},
                   {"Severity", message->second.severity},
                   {"MessageSeverity", message->second.messageSeverity},
                   {"NumberOfArgs", message->second.numberOfArgs},
                   {"Resolution", message->second.resolution}};
            if (message->second.numberOfArgs > 0)
            {
                nlohmann::json& messageParamArray = obj["ParamTypes"];
                for (const char* str : message->second.paramTypes)
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

} // namespace redfish
