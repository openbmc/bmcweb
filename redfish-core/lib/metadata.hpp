// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "http_request.hpp"
#include "persistent_data.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/systemd_utils.hpp"

#include <tinyxml2.h>

#include <nlohmann/json.hpp>

namespace redfish
{

inline std::string
    getMetadataPieceForFile(const std::filesystem::path& filename)
{
    std::string xml;
    tinyxml2::XMLDocument doc;
    std::string pathStr = filename.string();
    if (doc.LoadFile(pathStr.c_str()) != tinyxml2::XML_SUCCESS)
    {
        BMCWEB_LOG_ERROR("Failed to open XML file {}", pathStr);
        return "";
    }
    xml += std::format("    <edmx:Reference Uri=\"/redfish/v1/schema/{}\">\n",
                       filename.filename().string());
    // std::string edmx = "{http://docs.oasis-open.org/odata/ns/edmx}";
    // std::string edm = "{http://docs.oasis-open.org/odata/ns/edm}";
    const char* edmx = "edmx:Edmx";
    for (tinyxml2::XMLElement* edmxNode = doc.FirstChildElement(edmx);
         edmxNode != nullptr; edmxNode = edmxNode->NextSiblingElement(edmx))
    {
        const char* dataServices = "edmx:DataServices";
        for (tinyxml2::XMLElement* node =
                 edmxNode->FirstChildElement(dataServices);
             node != nullptr; node = node->NextSiblingElement(dataServices))
        {
            BMCWEB_LOG_DEBUG("Got data service for {}", pathStr);
            const char* schemaTag = "Schema";
            for (tinyxml2::XMLElement* schemaNode =
                     node->FirstChildElement(schemaTag);
                 schemaNode != nullptr;
                 schemaNode = schemaNode->NextSiblingElement(schemaTag))
            {
                std::string ns = schemaNode->Attribute("Namespace");
                // BMCWEB_LOG_DEBUG("Found namespace {}", ns);
                std::string alias;
                if (std::string_view(ns).starts_with("RedfishExtensions"))
                {
                    alias = " Alias=\"Redfish\"";
                }
                xml += std::format(
                    "        <edmx:Include Namespace=\"{}\"{}/>\n", ns, alias);
            }
        }
    }
    xml += "    </edmx:Reference>\n";
    return xml;
}

inline void
    handleMetadataGet(App& /*app*/, const crow::Request& /*req*/,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    std::filesystem::path schema("/usr/share/www/redfish/v1/schema");
    std::error_code ec;
    auto iter = std::filesystem::directory_iterator(schema, ec);
    if (ec)
    {
        BMCWEB_LOG_ERROR("Failed to open XML folder {}", schema.string());
        asyncResp->res.result(
            boost::beast::http::status::internal_server_error);
        return;
    }
    std::string xml;

    xml += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    xml +=
        "<edmx:Edmx xmlns:edmx=\"http://docs.oasis-open.org/odata/ns/edmx\" Version=\"4.0\">\n";
    for (const auto& dirEntry : iter)
    {
        std::string path = dirEntry.path().filename();
        if (!std::string_view(path).ends_with("_v1.xml"))
        {
            continue;
        }
        std::string metadataPiece = getMetadataPieceForFile(dirEntry.path());
        if (metadataPiece.empty())
        {
            asyncResp->res.result(
                boost::beast::http::status::internal_server_error);
            return;
        }
        xml += metadataPiece;
    }
    xml += "    <edmx:DataServices>\n";
    xml +=
        "        <Schema xmlns=\"http://docs.oasis-open.org/odata/ns/edm\" Namespace=\"Service\">\n";
    xml +=
        "            <EntityContainer Name=\"Service\" Extends=\"ServiceRoot.v1_0_0.ServiceContainer\"/>\n";
    xml += "        </Schema>\n";
    xml += "    </edmx:DataServices>\n";
    xml += "</edmx:Edmx>\n";

    asyncResp->res.addHeader(boost::beast::http::field::content_type,
                             "application/xml");
    asyncResp->res.write(std::move(xml));
}

inline void requestRoutesMetadata(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/$metadata")
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleMetadataGet, std::ref(app)));
}

} // namespace redfish
