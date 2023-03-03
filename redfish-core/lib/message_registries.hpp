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

#include "app.hpp"
#include "query.hpp"
#include "registries.hpp"
#include "registries/base_message_registry.hpp"
#include "registries/openbmc_message_registry.hpp"
#include "registries/privilege_registry.hpp"
#include "registries/resource_event_message_registry.hpp"
#include "registries/task_event_message_registry.hpp"

#include <array>

namespace redfish
{

inline void handleMessageRegistryFileCollectionGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    // Collections don't include the static data added by SubRoute
    // because it has a duplicate entry for members

    asyncResp->res.jsonValue["@odata.type"] =
        "#MessageRegistryFileCollection.MessageRegistryFileCollection";
    asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Registries";
    asyncResp->res.jsonValue["Name"] = "MessageRegistryFile Collection";
    asyncResp->res.jsonValue["Description"] =
        "Collection of MessageRegistryFiles";
    asyncResp->res.jsonValue["Members@odata.count"] = 4;

    nlohmann::json& members = asyncResp->res.jsonValue["Members"];
    for (const char* memberName :
         std::to_array({"Base", "TaskEvent", "ResourceEvent", "OpenBMC"}))
    {
        nlohmann::json::object_t member;
        member["@odata.id"] = crow::utility::urlFromPieces(
            "redfish", "v1", "Registries", memberName);
        members.emplace_back(std::move(member));
    }
}

inline void requestRoutesMessageRegistryFileCollection(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Registries/")
        .privileges(redfish::privileges::getMessageRegistryFileCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleMessageRegistryFileCollectionGet, std::ref(app)));
}

inline void handleMessageRoutesMessageRegistryFileGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& registry)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    const registries::Header* header = nullptr;
    std::string dmtf = "DMTF ";
    std::string_view url;

    if (registry == "Base")
    {
        header = &registries::base::header;
        url = registries::base::url;
    }
    else if (registry == "TaskEvent")
    {
        header = &registries::task_event::header;
        url = registries::task_event::url;
    }
    else if (registry == "OpenBMC")
    {
        header = &registries::openbmc::header;
        dmtf.clear();
    }
    else if (registry == "ResourceEvent")
    {
        header = &registries::resource_event::header;
        url = registries::resource_event::url;
    }
    else
    {
        messages::resourceNotFound(asyncResp->res, "MessageRegistryFile",
                                   registry);
        return;
    }

    asyncResp->res.jsonValue["@odata.id"] =
        crow::utility::urlFromPieces("redfish", "v1", "Registries", registry);
    asyncResp->res.jsonValue["@odata.type"] =
        "#MessageRegistryFile.v1_1_0.MessageRegistryFile";
    asyncResp->res.jsonValue["Name"] = registry + " Message Registry File";
    asyncResp->res.jsonValue["Description"] =
        dmtf + registry + " Message Registry File Location";
    asyncResp->res.jsonValue["Id"] = header->registryPrefix;
    asyncResp->res.jsonValue["Registry"] = header->id;
    nlohmann::json::array_t languages;
    languages.push_back("en");
    asyncResp->res.jsonValue["Languages@odata.count"] = languages.size();
    asyncResp->res.jsonValue["Languages"] = std::move(languages);
    nlohmann::json::array_t locationMembers;
    nlohmann::json::object_t location;
    location["Language"] = "en";
    location["Uri"] = "/redfish/v1/Registries/" + registry + "/" + registry;

    if (!url.empty())
    {
        location["PublicationUri"] = url;
    }
    locationMembers.emplace_back(std::move(location));
    asyncResp->res.jsonValue["Location@odata.count"] = locationMembers.size();
    asyncResp->res.jsonValue["Location"] = std::move(locationMembers);
}

inline void requestRoutesMessageRegistryFile(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Registries/<str>/")
        .privileges(redfish::privileges::getMessageRegistryFile)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleMessageRoutesMessageRegistryFileGet, std::ref(app)));
}

inline void handleMessageRegistryGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& registry, const std::string& registryMatch)

{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    const registries::Header* header = nullptr;
    std::vector<const registries::MessageEntry*> registryEntries;
    if (registry == "Base")
    {
        header = &registries::base::header;
        for (const registries::MessageEntry& entry : registries::base::registry)
        {
            registryEntries.emplace_back(&entry);
        }
    }
    else if (registry == "TaskEvent")
    {
        header = &registries::task_event::header;
        for (const registries::MessageEntry& entry :
             registries::task_event::registry)
        {
            registryEntries.emplace_back(&entry);
        }
    }
    else if (registry == "OpenBMC")
    {
        header = &registries::openbmc::header;
        for (const registries::MessageEntry& entry :
             registries::openbmc::registry)
        {
            registryEntries.emplace_back(&entry);
        }
    }
    else if (registry == "ResourceEvent")
    {
        header = &registries::resource_event::header;
        for (const registries::MessageEntry& entry :
             registries::resource_event::registry)
        {
            registryEntries.emplace_back(&entry);
        }
    }
    else
    {
        messages::resourceNotFound(asyncResp->res, "MessageRegistryFile",
                                   registry);
        return;
    }

    if (registry != registryMatch)
    {
        messages::resourceNotFound(asyncResp->res, header->type, registryMatch);
        return;
    }

    asyncResp->res.jsonValue["@Redfish.Copyright"] = header->copyright;
    asyncResp->res.jsonValue["@odata.type"] = header->type;
    asyncResp->res.jsonValue["Id"] = header->id;
    asyncResp->res.jsonValue["Name"] = header->name;
    asyncResp->res.jsonValue["Language"] = header->language;
    asyncResp->res.jsonValue["Description"] = header->description;
    asyncResp->res.jsonValue["RegistryPrefix"] = header->registryPrefix;
    asyncResp->res.jsonValue["RegistryVersion"] = header->registryVersion;
    asyncResp->res.jsonValue["OwningEntity"] = header->owningEntity;

    nlohmann::json& messageObj = asyncResp->res.jsonValue["Messages"];

    // Go through the Message Registry and populate each Message
    for (const registries::MessageEntry* message : registryEntries)
    {
        nlohmann::json& obj = messageObj[message->first];
        obj["Description"] = message->second.description;
        obj["Message"] = message->second.message;
        obj["Severity"] = message->second.messageSeverity;
        obj["MessageSeverity"] = message->second.messageSeverity;
        obj["NumberOfArgs"] = message->second.numberOfArgs;
        obj["Resolution"] = message->second.resolution;
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
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleMessageRegistryGet, std::ref(app)));
}
} // namespace redfish
