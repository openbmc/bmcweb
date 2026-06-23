// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "xml_parser.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>

#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace redfish
{

inline std::string getMetadataPieceForFile(
    const std::filesystem::path& filename)
{
    std::string xml;
    std::string pathStr = filename.string();
    std::ifstream file(pathStr);
    if (!file)
    {
        BMCWEB_LOG_ERROR("Failed to open XML file {}", pathStr);
        return "";
    }
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    std::optional<xml::Element> doc = xml::parse(content);
    if (!doc || doc->name != "edmx:Edmx")
    {
        BMCWEB_LOG_ERROR("Failed to parse XML file {}", pathStr);
        return "";
    }
    xml += std::format("    <edmx:Reference Uri=\"/redfish/v1/schema/{}\">\n",
                       filename.filename().string());
    for (const xml::Element& node : doc->childrenNamed("edmx:DataServices"))
    {
        BMCWEB_LOG_DEBUG("Got data service for {}", pathStr);
        for (const xml::Element& schemaNode : node.childrenNamed("Schema"))
        {
            const std::string* ns = schemaNode.attribute("Namespace");
            if (ns == nullptr)
            {
                continue;
            }
            // BMCWEB_LOG_DEBUG("Found namespace {}", *ns);
            std::string alias;
            if (std::string_view(*ns).starts_with("RedfishExtensions"))
            {
                alias = " Alias=\"Redfish\"";
            }
            xml += std::format("        <edmx:Include Namespace=\"{}\"{}/>\n",
                               *ns, alias);
        }
    }
    xml += "    </edmx:Reference>\n";
    return xml;
}

inline void handleMetadataGet(
    App& /*app*/, const crow::Request& /*req*/,
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
    BMCWEB_ROUTE(app, "/redfish/v1/$metadata/")
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleMetadataGet, std::ref(app)));
}

} // namespace redfish
