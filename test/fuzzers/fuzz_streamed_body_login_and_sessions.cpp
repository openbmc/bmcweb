// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026, Oracle and/or its affiliates. All rights reserved.
//
// LibFuzzer harness to better simulate *streamed* HTTP request body ingestion
// for unauthenticated endpoints:
//   - POST /login (JSON path)
//   - POST /redfish/v1/SessionService/Sessions (+ /Sessions/Members)
//
// This harness:
//   1) streams the body into bmcweb::HttpBody::reader with adversarial chunking
//   2) builds a boost::beast request and then a crow::Request
//   3) invokes the same JSON parsing/extraction helpers as production code
//
// Goal: increase realism for large payload uploads and chunk boundary bugs.

#include "http/http_body.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "multipart_parser.hpp"
#include "utils/json_utils.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace
{

// Simple deterministic byte reader for chunk sizes / routing decisions.
struct FuzzCursor
{
    const uint8_t* data;
    size_t size;
    size_t off = 0;

    uint8_t getByte(uint8_t fallback = 0)
    {
        if (off >= size)
        {
            return fallback;
        }
        return data[off++];
    }

    std::span<const uint8_t> getSpan(size_t n)
    {
        if (off > size)
        {
            return {};
        }
        size_t remain = size - off;
        size_t take = (n > remain) ? remain : n;
        std::span<const uint8_t> out(data + off, take);
        off += take;
        return out;
    }

};

static crow::Request makeStreamedRequest(std::string_view target,
                                        boost::beast::http::verb method,
                                        std::string_view contentType,
                                        std::span<const uint8_t> body,
                                        std::span<const uint8_t> chunkControl)
{
    // Build a beast request of the same body type bmcweb uses.
    crow::Request::Body beastReq;
    beastReq.method(method);
    beastReq.target(target);
    beastReq.set(boost::beast::http::field::content_type, contentType);

    // Stream body into HttpBody via its reader.
    bmcweb::HttpBody::reader reader(beastReq.base(), beastReq.body());
    boost::beast::error_code ec;
    reader.init(static_cast<std::uint64_t>(body.size()), ec);

    // Adversarial chunking: chunk sizes are derived from chunkControl bytes.
    size_t pos = 0;
    size_t controlPos = 0;
    while (pos < body.size())
    {
        uint8_t c = 1;
        if (controlPos < chunkControl.size())
        {
            c = chunkControl[controlPos++];
        }
        // 1..4096 byte chunks (bias toward small chunks)
        size_t chunkLen = (static_cast<size_t>(c) % 64) + 1;
        chunkLen = chunkLen * 64;
        if (chunkLen > 4096)
        {
            chunkLen = 4096;
        }
        size_t remain = body.size() - pos;
        if (chunkLen > remain)
        {
            chunkLen = remain;
        }

        auto buf = boost::asio::buffer(body.data() + pos, chunkLen);
        boost::system::error_code putEc;
        (void)reader.put(buf, putEc);
        pos += chunkLen;
    }

    bmcweb::HttpBody::reader::finish(ec);

    // Now wrap as crow::Request.
    std::error_code reqEc;
    crow::Request req(std::move(beastReq), reqEc);
    return req;
}

static void parseLoginJson(const crow::Request& req)
{
    // Mirrors bmcweb/features/webui_login/login_routes.hpp JSON path.
    std::string_view contentType = req.getHeaderValue("content-type");
    if (!contentType.starts_with("application/json"))
    {
        return;
    }

    nlohmann::json loginCredentials = nlohmann::json::parse(req.body(), nullptr,
                                                           false);
    if (loginCredentials.is_discarded())
    {
        return;
    }
    // Touch some fields similar to the production handler.
    auto userIt = loginCredentials.find("username");
    auto passIt = loginCredentials.find("password");
    if (userIt != loginCredentials.end() && passIt != loginCredentials.end())
    {
        const std::string* userStr = userIt->get_ptr<const std::string*>();
        const std::string* passStr = passIt->get_ptr<const std::string*>();
        if (userStr != nullptr && passStr != nullptr)
        {
            [[maybe_unused]] volatile size_t u = userStr->size();
            [[maybe_unused]] volatile size_t p = passStr->size();
        }
    }
}

static void parseLoginMultipart(const crow::Request& req)
{
    // Drive the same custom multipart parser used by /login when
    // Content-Type is multipart/form-data.
    std::string_view contentType = req.getHeaderValue("content-type");
    if (!contentType.starts_with("multipart/form-data"))
    {
        return;
    }

    MultipartParser mp;
    if (mp.parse(contentType, req.body()) != ParserError::PARSER_SUCCESS)
    {
        return;
    }

    // Touch a few common field names that the login route may look at.
    // We just want to exercise parsing + map storage.
    for (const FormPart& part : mp.mime_fields)
    {
        // Touch content-disposition/name and content length.
        std::string_view disp = part.fields[boost::beast::http::field::content_disposition];
        [[maybe_unused]] volatile size_t d = disp.size();
        [[maybe_unused]] volatile size_t c = part.content.size();
    }
}

static void parseSessionServiceJson(const crow::Request& req)
{
    // Mirrors redfish-core/lib/redfish_sessions.hpp parsing call.
    crow::Response res;
    std::string username;
    std::string password;
    std::optional<std::string> clientId;
    std::optional<std::string> token;

    (void)redfish::json_util::readJsonPatch(req, res, "Context", clientId,
                                           "Password", password, "Token",
                                           token, "UserName", username);
    [[maybe_unused]] volatile size_t u = username.size();
    [[maybe_unused]] volatile size_t p = password.size();
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    try
    {
        FuzzCursor cur{data, size};

        // First byte chooses endpoint/mode.
        // 0: /login JSON
        // 1: SessionService JSON
        // 2: /login multipart/form-data
        uint8_t mode = cur.getByte(0);
        // Next byte influences how many bytes go to chunk-control.
        uint8_t ctlLenByte = cur.getByte(16);
        size_t ctlLen = (static_cast<size_t>(ctlLenByte) % 64) + 1; // 1..64

        std::span<const uint8_t> chunkCtl = cur.getSpan(ctlLen);
        std::span<const uint8_t> body = cur.getSpan(size - cur.off);

        // Construct and parse.
        if ((mode % 3) == 0)
        {
            // /login
            crow::Request req = makeStreamedRequest(
                "/login", boost::beast::http::verb::post,
                "application/json", body, chunkCtl);
            // ensure both header casings exist (bmcweb checks both in places)
            req.addHeader("Content-Type", "application/json");
            req.addHeader("content-type", "application/json");
            parseLoginJson(req);
        }
        else if ((mode % 3) == 1)
        {
            // SessionService: exercise both targets
            {
                crow::Request req = makeStreamedRequest(
                    "/redfish/v1/SessionService/Sessions/",
                    boost::beast::http::verb::post, "application/json", body,
                    chunkCtl);
                req.addHeader("Content-Type", "application/json");
                req.addHeader("content-type", "application/json");
                parseSessionServiceJson(req);
            }
            {
                crow::Request req = makeStreamedRequest(
                    "/redfish/v1/SessionService/Sessions/Members/",
                    boost::beast::http::verb::post, "application/json", body,
                    chunkCtl);
                req.addHeader("Content-Type", "application/json");
                req.addHeader("content-type", "application/json");
                parseSessionServiceJson(req);
            }
        }
        else
        {
            // /login multipart/form-data
            // We need a boundary; derive one from chunkCtl bytes.
            std::string boundary;
            boundary.reserve(16);
            for (size_t i = 0; i < chunkCtl.size() && boundary.size() < 12;
                 i++)
            {
                char ch = static_cast<char>('a' + (chunkCtl[i] % 26));
                boundary.push_back(ch);
            }
            if (boundary.empty())
            {
                boundary = "b";
            }
            std::string ct =
                std::string("multipart/form-data; boundary=") + boundary;

            crow::Request req = makeStreamedRequest(
                "/login", boost::beast::http::verb::post, ct, body, chunkCtl);
            req.addHeader("Content-Type", ct);
            req.addHeader("content-type", ct);
            parseLoginMultipart(req);
        }
    }
    catch (...)
    {
        // swallow
    }
    return 0;
}
