// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026, Oracle and/or its affiliates. All rights reserved.
//
// Fuzz harness for the JSON parsing utilities used by
// POST /redfish/v1/SessionService/Sessions (and /Sessions/Members).
//
// This exercises the same JSON parsing + key extraction helpers as the real
// endpoint but intentionally avoids calling PAM.

#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "utils/json_utils.hpp"

#include <boost/beast/http/verb.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

static void parseRedfishSessionCreateJson(const crow::Request& req)
{
    crow::Response res;

    std::string username;
    std::string password;
    std::optional<std::string> clientId;
    std::optional<std::string> token;

    // Mirrors redfish-core/lib/redfish_sessions.hpp
    (void)redfish::json_util::readJsonPatch( //
        req, res,                            //
        "Context", clientId,                //
        "Password", password,               //
        "Token", token,                     //
        "UserName", username                //
    );

    // Prevent optimizing away.
    [[maybe_unused]] volatile size_t u = username.size();
    [[maybe_unused]] volatile size_t p = password.size();
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    try
    {
        std::string body(reinterpret_cast<const char*>(data), size);
        std::error_code ec;

        crow::Request req(std::string_view(body), ec);
        req.method(boost::beast::http::verb::post);
        req.addHeader("content-type", "application/json");
        req.addHeader("Content-Type", "application/json");

        // Exercise both route targets; parsing code is identical.
        req.target("/redfish/v1/SessionService/Sessions/");
        parseRedfishSessionCreateJson(req);

        req.target("/redfish/v1/SessionService/Sessions/Members/");
        parseRedfishSessionCreateJson(req);
    }
    catch (...)
    {
        // Don't let exceptions escape libFuzzer entrypoint.
    }
    return 0;
}
