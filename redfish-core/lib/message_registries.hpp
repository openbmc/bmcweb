// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2019 Intel Corporation
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "query.hpp"
#include "registries.hpp"
#include "registries/privilege_registry.hpp"

#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>

#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <utility>

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

    nlohmann::json& members = asyncResp->res.jsonValue["Members"];

    for (const auto& memberName : std::views::keys(registries::allRegistries()))
    {
        nlohmann::json::object_t member;
        member["@odata.id"] =
            boost::urls::format("/redfish/v1/Registries/{}", memberName);
        members.emplace_back(std::move(member));
    }
    asyncResp->res.jsonValue["Members@odata.count"] = members.size();
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
    std::string dmtf = "DMTF ";
    std::optional<registries::RegistryEntryRef> registryEntry =
        registries::getRegistryFromPrefix(registry);

    if (!registryEntry)
    {
        messages::resourceNotFound(asyncResp->res, "MessageRegistryFile",
                                   registry);
        return;
    }
    if (registry == "OpenBMC")
    {
        dmtf.clear();
    }
    const registries::Header& header = registryEntry->get().header;
    const char* url = registryEntry->get().url;

    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Registries/{}", registry);
    asyncResp->res.jsonValue["@odata.type"] =
        "#MessageRegistryFile.v1_1_0.MessageRegistryFile";
    asyncResp->res.jsonValue["Name"] = registry + " Message Registry File";
    asyncResp->res.jsonValue["Description"] =
        dmtf + registry + " Message Registry File Location";
    asyncResp->res.jsonValue["Id"] = header.registryPrefix;
    asyncResp->res.jsonValue["Registry"] =
        std::format("{}.{}.{}", header.registryPrefix, header.versionMajor,
                    header.versionMinor);
    nlohmann::json::array_t languages;
    languages.emplace_back(header.language);
    asyncResp->res.jsonValue["Languages@odata.count"] = languages.size();
    asyncResp->res.jsonValue["Languages"] = std::move(languages);
    nlohmann::json::array_t locationMembers;
    nlohmann::json::object_t location;
    location["Language"] = header.language;
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

    std::optional<registries::RegistryEntryRef> registryEntry =
        registries::getRegistryFromPrefix(registry);
    if (!registryEntry)
    {
        messages::resourceNotFound(asyncResp->res, "MessageRegistryFile",
                                   registry);
        return;
    }

    const registries::Header& header = registryEntry->get().header;
    if (registry != registryMatch)
    {
        messages::resourceNotFound(asyncResp->res, header.type, registryMatch);
        return;
    }

    asyncResp->res.jsonValue["@Redfish.Copyright"] = header.copyright;
    asyncResp->res.jsonValue["@odata.type"] = header.type;
    asyncResp->res.jsonValue["Id"] =
        std::format("{}.{}.{}.{}", header.registryPrefix, header.versionMajor,
                    header.versionMinor, header.versionPatch);
    asyncResp->res.jsonValue["Name"] = header.name;
    asyncResp->res.jsonValue["Language"] = header.language;
    asyncResp->res.jsonValue["Description"] = header.description;
    asyncResp->res.jsonValue["RegistryPrefix"] = header.registryPrefix;
    asyncResp->res.jsonValue["RegistryVersion"] =
        std::format("{}.{}.{}", header.versionMajor, header.versionMinor,
                    header.versionPatch);
    asyncResp->res.jsonValue["OwningEntity"] = header.owningEntity;

    nlohmann::json& messageObj = asyncResp->res.jsonValue["Messages"];

    // Go through the Message Registry and populate each Message
    const registries::MessageEntries registryEntries =
        registries::getRegistryMessagesFromPrefix(registry);

    for (const registries::MessageEntry& message : registryEntries)
    {
        nlohmann::json& obj = messageObj[message.first];
        obj["Description"] = message.second.description;
        obj["Message"] = message.second.message;
        obj["Severity"] = message.second.messageSeverity;
        obj["MessageSeverity"] = message.second.messageSeverity;
        obj["NumberOfArgs"] = message.second.numberOfArgs;
        obj["Resolution"] = message.second.resolution;
        if (message.second.numberOfArgs > 0)
        {
            nlohmann::json& messageParamArray = obj["ParamTypes"];
            messageParamArray = nlohmann::json::array();
            for (const char* str : message.second.paramTypes)
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
