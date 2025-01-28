// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2019 Intel Corporation
#pragma once

#include "app.hpp"
#include "query.hpp"
#include "registries.hpp"
#include "registries_selector.hpp"

#include <boost/url/format.hpp>

#include <array>
#include <format>

namespace redfish
{

static constexpr const char* attributeRegistryDirPath =
    "/var/lib/bmcweb/bios-registries/";

inline void populateAttributeRegistries(nlohmann::json& members)
{
    std::error_code ec;
    auto attributeRegistryFiles =
        std::filesystem::directory_iterator(attributeRegistryDirPath, ec);
    if (!ec)
    {
        for (const auto& attributeRegistryFile : attributeRegistryFiles)
        {
            nlohmann::json::object_t member;
            member["@odata.id"] = boost::urls::format(
                "/redfish/v1/Registries/{}",
                attributeRegistryFile.path().stem().string());
            members.emplace_back(std::move(member));
        }
    }
}

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

    static constexpr const auto registryFiles = std::to_array(
        {"Base", "TaskEvent", "ResourceEvent", "OpenBMC", "Telemetry",
         "HeartbeatEvent"});

    for (const char* memberName : registryFiles)
    {
        nlohmann::json::object_t member;
        member["@odata.id"] =
            boost::urls::format("/redfish/v1/Registries/{}", memberName);
        members.emplace_back(std::move(member));
    }

    populateAttributeRegistries(members);

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
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& registry, const registries::HeaderAndUrl& headerAndUrl)
{
    std::string dmtf = "DMTF ";
    if (registry == "OpenBMC")
    {
        dmtf.clear();
    }
    const registries::Header& header = headerAndUrl.header;
    const char* url = headerAndUrl.url;

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

inline void handleAttributeRegistryFileGet(crow::Response& response,
                                           const std::string& registry)
{
    response.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Registries/{}", registry);
    response.jsonValue["@odata.type"] =
        "#MessageRegistryFile.v1_1_0.MessageRegistryFile";
    response.jsonValue["Name"] = registry + " Registry File";
    response.jsonValue["Description"] =
        registry + " Attribute Registry File Location";
    response.jsonValue["Id"] = registry;
    response.jsonValue["Registry"] = registry;
    nlohmann::json::array_t languages;
    languages.emplace_back("en");
    response.jsonValue["Languages@odata.count"] = languages.size();
    response.jsonValue["Languages"] = std::move(languages);
    nlohmann::json::array_t locationMembers;
    nlohmann::json::object_t location;
    location["Language"] = "en";
    location["Uri"] = "/redfish/v1/Registries/" + registry + "/" + registry;
    locationMembers.emplace_back(std::move(location));
    response.jsonValue["Location@odata.count"] = locationMembers.size();
    response.jsonValue["Location"] = std::move(locationMembers);
}

inline std::filesystem::path
    getPathForAttributeRegistry(const std::string& registry)
{
    std::filesystem::path path;
    std::error_code ec;
    auto attributeRegistryFiles =
        std::filesystem::directory_iterator(attributeRegistryDirPath, ec);
    if (!ec)
    {
        for (const auto& attributeRegistryFile : attributeRegistryFiles)
        {
            if (attributeRegistryFile.path().stem().string() == registry)
            {
                path = attributeRegistryFile.path();
                break;
            }
        }
    }
    return path;
}

inline void
    handleRegistryFileGet(crow::App& app, const crow::Request& req,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& registry)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    std::optional<registries::HeaderAndUrl> headerAndUrl =
        registries::getRegistryHeaderAndUrlFromPrefix(registry);

    if (headerAndUrl)
    {
        handleMessageRoutesMessageRegistryFileGet(asyncResp, registry,
                                                  headerAndUrl.value());
    }
    else
    {
        std::filesystem::path attributeRegistryPath =
            getPathForAttributeRegistry(registry);
        if (!attributeRegistryPath.empty())
        {
            handleAttributeRegistryFileGet(std::ref(asyncResp->res), registry);
        }
        else
        {
            messages::resourceNotFound(asyncResp->res, "MessageRegistryFile",
                                       registry);
        }
    }
}

inline void requestRoutesMessageRegistryFile(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Registries/<str>/")
        .privileges(redfish::privileges::getMessageRegistryFile)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleRegistryFileGet, std::ref(app)));
}

inline void handleMessageRegistryGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& registry, const std::string& registryMatch,
    const registries::HeaderAndUrl& headerAndUrl)

{
    const registries::Header& header = headerAndUrl.header;
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
    const std::span<const registries::MessageEntry> registryEntries =
        registries::getRegistryFromPrefix(registry);

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

inline void handleRegistryGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& registry, const std::string& registryMatch)

{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    std::optional<registries::HeaderAndUrl> headerAndUrl =
        registries::getRegistryHeaderAndUrlFromPrefix(registry);

    if (headerAndUrl)
    {
        handleMessageRegistryGet(asyncResp, registry, registryMatch,
                                 headerAndUrl.value());
    }
    else
    {
        std::filesystem::path attributeRegistryPath =
            getPathForAttributeRegistry(registry);
        if (!attributeRegistryPath.empty())
        {
            if (registry != registryMatch ||
                asyncResp->res.openFile(attributeRegistryPath) !=
                    crow::OpenCode::Success)
            {
                messages::resourceNotFound(asyncResp->res, "AttributeRegistry",
                                           registryMatch);
                return;
            }
            asyncResp->res.addHeader(boost::beast::http::field::content_type,
                                     "application/json");
        }
        else
        {
            messages::resourceNotFound(asyncResp->res, "MessageRegistryFile",
                                       registry);
        }
    }
}

inline void requestRoutesMessageRegistry(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Registries/<str>/<str>/")
        .privileges(redfish::privileges::getMessageRegistryFile)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleRegistryGet, std::ref(app)));
}
} // namespace redfish
