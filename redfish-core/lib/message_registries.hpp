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

#include "registries.hpp"
#include "registries/base_message_registry.hpp"
#include "registries/openbmc_message_registry.hpp"
#include "registries/resource_event_message_registry.hpp"
#include "registries/task_event_message_registry.hpp"

#include <app.hpp>
#include <registries/privilege_registry.hpp>

namespace redfish
{

inline void handleMessageRegistryFileCollectionGet(
    const crow::Request&, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    // Collections don't include the static data added by SubRoute
    // because it has a duplicate entry for members

    asyncResp->res.jsonValue = {
        {"@odata.type", "#MessageRegistryFileCollection."
                        "MessageRegistryFileCollection"},
        {"@odata.id", "/redfish/v1/Registries"},
        {"Name", "MessageRegistryFile Collection"},
        {"Description", "Collection of MessageRegistryFiles"},
        {"Members@odata.count", 5},
        {"Members",
         {{{"@odata.id", "/redfish/v1/Registries/Base"}},
          {{"@odata.id", "/redfish/v1/Registries/TaskEvent"}},
          {{"@odata.id", "/redfish/v1/Registries/ResourceEvent"}},
          {{"@odata.id", "/redfish/v1/Registries/OpenBMC"}},
          {{"@odata.id", "/redfish/v1/Registries/PrivilegeRegistry"}}}}};
}

inline void requestRoutesMessageRegistryFileCollection(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Registries/")
        .privileges(redfish::privileges::getMessageRegistryFileCollection)
        .methods(boost::beast::http::verb::get)(
            handleMessageRegistryFileCollectionGet);
}

inline void handleMessageRoutesMessageRegistryFileGet(
    const crow::Request&, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& registry)
{
    const message_registries::Header* header;
    redfish::message_registries::Header privilege_header = {};
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
    else if (registry == "PrivilegeRegistry")
    {
        privilege_header = {
            "Copyright 2014-2021 DMTF. All rights reserved.",
            "#MessageRegistry.v1_4_0.MessageRegistry",
            "Redfish_1.1.0_PrivilegeRegistry",
            "Privilege Registry",
            "en",
            "This registry defines the Privileges for Redfish Resources",
            "PrivilegeRegistry",
            "1.1.0",
            "DMTF",
        };
        header = &privilege_header;
        dmtf.clear();
    }

    else
    {
        messages::resourceNotFound(
            asyncResp->res, "#MessageRegistryFile.v1_1_0.MessageRegistryFile",
            registry);
        return;
    }

    if (registry == "PrivilegeRegistry")
    {
        asyncResp->res.jsonValue = {
            {"@odata.id", "/redfish/v1/Registries/" + registry},
            {"@odata.type", "#MessageRegistryFile.v1_1_0.MessageRegistryFile"},
            {"Name", registry + " Message Registry File"},
            {"Description", dmtf + registry + "Message Registry File Location"},
            {"Id", header->registryPrefix},
            {"Registry", header->id},
            {"Languages", {"en"}},
            {"Languages@odata.count", 1},
            {"Location",
             {{{"Language", "en"},
               {"Uri", "/redfish/v1/Registries/" + registry +
                           "/Redfish_1.1.0_PrivilegeRegistry.json"}}}},
            {"Location@odata.count", 1}};
    }
    else
    {
        asyncResp->res.jsonValue = {
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
    }
    if (url != nullptr)
    {
        asyncResp->res.jsonValue["Location"][0]["PublicationUri"] = url;
    }
}

inline void requestRoutesMessageRegistryFile(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Registries/<str>/")
        .privileges(redfish::privileges::getMessageRegistryFile)
        .methods(boost::beast::http::verb::get)(
            handleMessageRoutesMessageRegistryFileGet);
}

inline void handleMessageRegistryGet(
    const crow::Request&, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& registry, const std::string& registryMatch)

{
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
            asyncResp->res, "#MessageRegistryFile.v1_1_0.MessageRegistryFile",
            registry);
        return;
    }

    if (registry != registryMatch)
    {
        messages::resourceNotFound(asyncResp->res, header->type, registryMatch);
        return;
    }

    asyncResp->res.jsonValue = {{"@Redfish.Copyright", header->copyright},
                                {"@odata.type", header->type},
                                {"Id", header->id},
                                {"Name", header->name},
                                {"Language", header->language},
                                {"Description", header->description},
                                {"RegistryPrefix", header->registryPrefix},
                                {"RegistryVersion", header->registryVersion},
                                {"OwningEntity", header->owningEntity}};

    nlohmann::json& messageObj = asyncResp->res.jsonValue["Messages"];

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
            messageParamArray = nlohmann::json::array();
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
}

inline void requestRoutesMessageRegistry(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Registries/<str>/<str>/")
        .privileges(redfish::privileges::getMessageRegistryFile)
        .methods(boost::beast::http::verb::get)(handleMessageRegistryGet);
}
} // namespace redfish
