#pragma once

#include <string>
#include "http_response.hpp"

namespace redfish {
inline void declareResource(crow::Response& res, std::string_view schemaNamespace, std::string_view version, std::string_view schemaName){
  std::string type;
  type.reserve(schemaNamespace.size() + version.size() + schemaName.size() + 4);
  type += '#';
  type += schemaNamespace;
  type += '.';
  type += 'v';
  type += version;
  type += '.';
  type += schemaName;
  res.jsonValue["@odata.type"] = std::move(type);


  std::string link("</redfish/v1/JsonSchemas/");
  link += schemaNamespace;
  link += '/';
  link += schemaNamespace;
  link += ".json>; rel=describedby";
}
}
