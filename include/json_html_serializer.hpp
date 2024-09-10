// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "http_response.hpp"

#include <nlohmann/json.hpp>

#include <string>

namespace json_html_util
{

void dumpHtml(std::string& out, const nlohmann::json& json);
void prettyPrintJson(crow::Response& res);

} // namespace json_html_util
