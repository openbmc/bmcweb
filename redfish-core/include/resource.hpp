#pragma once

#include "http_response.hpp"

#include <string>

namespace redfish
{

inline void declareResourceOdata(nlohmann::json& json,
                                 std::string_view schemaNamespace,
                                 std::string_view version,
                                 std::string_view schemaName)
{
    std::string type;
    type.reserve(schemaNamespace.size() + version.size() + schemaName.size() +
                 4);
    type += '#';
    type += schemaNamespace;
    type += '.';
    type += 'v';
    type += version;
    type += '.';
    type += schemaName;
    json["@odata.type"] = std::move(type);
}

inline void declareResource(crow::Response& res,
                            const nlohmann::json::json_pointer& pointer,
                            std::string_view schemaNamespace,
                            std::string_view version,
                            std::string_view schemaName)
{
    declareResourceOdata(res.jsonValue[pointer], schemaNamespace, version,
                         schemaName);

    std::string link("</redfish/v1/JsonSchemas/");
    link += schemaNamespace;
    link += '/';
    link += schemaNamespace;
    link += ".json>; rel=describedby";
    res.addHeader(boost::beast::http::field::link, link);
}
inline void declareResource(crow::Response& res,
                            std::string_view schemaNamespace,
                            std::string_view version,
                            std::string_view schemaName)
{
    declareResource(res, nlohmann::json::json_pointer("/"), schemaNamespace,
                    version, schemaName);
}
} // namespace redfish
