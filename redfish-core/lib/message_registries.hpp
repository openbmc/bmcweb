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

#include <boost/url/format.hpp>

#include <array>

namespace redfish
{
struct MessageRegistries
{
    using MessageFileGet = std::function<void(const registries::Header*& header,
                                              const char*& url)>;
    using MessageGet = std::function<void(
        const registries::Header*& header,
        std::vector<const registries::MessageEntry*>& entries)>;
    using MessageMap =
        std::unordered_map<std::string, std::pair<MessageFileGet, MessageGet>>;
    static auto makeFileGetter(auto& srcheader, auto& srcurl)
    {
        return [&srcheader, &srcurl](const registries::Header*& header,
                                     const char*& url) {
            header = &srcheader;
            url = srcurl;
        };
    }
    static auto makeMessageGetter(auto& srcheader, auto& registry)
    {
        return
            [&srcheader, &registry](
                const registries::Header*& header,
                std::vector<const registries::MessageEntry*>& registryEntries) {
            header = &srcheader;
            for (const auto& entry : registry)
            {
                registryEntries.emplace_back(&entry);
            }
        };
    }

  private:
    MessageMap messageRegistries;

    MessageRegistries()
    {
        registerMessage(
            "Base",
            makeFileGetter(registries::base::header, registries::base::url),
            makeMessageGetter(registries::base::header,
                              registries::base::registry));

        registerMessage("TaskEvent",
                        makeFileGetter(registries::task_event::header,
                                       registries::task_event::url),
                        makeMessageGetter(registries::task_event::header,
                                          registries::task_event::registry));

        registerMessage(
            "ResourceEvent",
            makeFileGetter(registries::resource_event::header,
                           registries::resource_event::url),
            makeMessageGetter(registries::resource_event::header,
                              registries::resource_event::registry));

        registerMessage("OpenBMC", MessageFileGet(),
                        makeMessageGetter(registries::openbmc::header,
                                          registries::openbmc::registry));
    }

  public:
    void registerMessage(const std::string& name, MessageFileGet&& fileGet,
                         MessageGet&& messageGet)
    {
        messageRegistries[name] = std::make_pair(std::move(fileGet),
                                                 std::move(messageGet));
    }
    void fillMemberNames(nlohmann::json& members)
    {
        for (const auto& [name, _] : messageRegistries)
        {
            nlohmann::json::object_t member;
            member["@odata.id"] =
                boost::urls::format("/redfish/v1/Registries/{}", name);
            members.emplace_back(std::move(member));
        }
    }
    void fillFileData(const std::string& name,
                      const registries::Header*& header, const char*& url)
    {
        auto it = messageRegistries.find(name);
        if (it != messageRegistries.end())
        {
            if (it->second.first)
            {
                it->second.first(header, url);
            }
        }
    }
    void fillMessageData(const std::string& name,
                         const registries::Header*& header,
                         std::vector<const registries::MessageEntry*>& entries)
    {
        auto it = messageRegistries.find(name);
        if (it != messageRegistries.end())
        {
            it->second.second(header, entries);
        }
    }
    static MessageRegistries& globalInstance()
    {
        static MessageRegistries global;
        return global;
    }
};
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
    MessageRegistries::globalInstance().fillMemberNames(members);
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
    const char* url = nullptr;

    if (registry == "OpenBMC")
    {
        header = &registries::openbmc::header;
        dmtf.clear();
    }
    MessageRegistries::globalInstance().fillFileData(registry, header, url);
    if (header == nullptr)
    {
        messages::resourceNotFound(asyncResp->res, "MessageRegistryFile",
                                   registry);
        return;
    }

    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Registries/{}", registry);
    asyncResp->res.jsonValue["@odata.type"] =
        "#MessageRegistryFile.v1_1_0.MessageRegistryFile";
    asyncResp->res.jsonValue["Name"] = registry + " Message Registry File";
    asyncResp->res.jsonValue["Description"] = dmtf + registry +
                                              " Message Registry File Location";
    asyncResp->res.jsonValue["Id"] = header->registryPrefix;
    asyncResp->res.jsonValue["Registry"] = header->id;
    nlohmann::json::array_t languages;
    languages.emplace_back("en");
    asyncResp->res.jsonValue["Languages@odata.count"] = languages.size();
    asyncResp->res.jsonValue["Languages"] = std::move(languages);
    nlohmann::json::array_t locationMembers;
    nlohmann::json::object_t location;
    location["Language"] = "en";
    location["Uri"] = "/redfish/v1/Registries/" + registry + "/" + registry;

    if (url != nullptr)
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
    MessageRegistries::globalInstance().fillMessageData(registry, header,
                                                        registryEntries);

    if (header == nullptr)
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
