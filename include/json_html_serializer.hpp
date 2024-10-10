#pragma once

#include "http_response.hpp"

#include <nlohmann/json.hpp>

#include <string>

namespace json_html_util
{

void dumpHtml(std::string& out, const nlohmann::json& json);
void prettyPrintJson(bmcweb::Response& res);

} // namespace json_html_util
