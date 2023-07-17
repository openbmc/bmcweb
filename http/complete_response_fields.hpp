#pragma once

#include "authentication.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "http_utility.hpp"
#include "json_html_serializer.hpp"
#include "logging.hpp"
#include "security_headers.hpp"
#include "utils/hex_utils.hpp"

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <nlohmann/json.hpp>

#include <array>

namespace crow
{
inline void
    completeResponseFieldsImpl(const Request& req, Response& res,
                               Response::string_body_response_type& /*unused*/)
{
    if (res.body().empty() && res.jsonValue.is_structured())
    {
        using http_helpers::ContentType;
        std::array<ContentType, 3> allowed{ContentType::CBOR, ContentType::JSON,
                                           ContentType::HTML};
        ContentType prefered =
            getPreferedContentType(req.getHeaderValue("Accept"), allowed);

        if (prefered == ContentType::HTML)
        {
            json_html_util::prettyPrintJson(res);
        }
        else if (prefered == ContentType::CBOR)
        {
            res.addHeader(boost::beast::http::field::content_type,
                          "application/cbor");
            nlohmann::json::to_cbor(res.jsonValue, res.body());
        }
        else
        {
            // Technically prefered could also be NoMatch here, but we'd
            // like to default to something rather than return 400 for
            // backward compatibility.
            res.addHeader(boost::beast::http::field::content_type,
                          "application/json");
            res.body() = res.jsonValue.dump(
                2, ' ', true, nlohmann::json::error_handler_t::replace);
        }
    }
}
inline void
    completeResponseFieldsImpl(const Request& /*unused*/, Response& res,
                               Response::file_body_response_type& /*unused*/)
{
    res.addHeader(boost::beast::http::field::content_type,
                  "application/octet-stream");
}

inline void completeResponseFields(const Request& req, Response& res)
{
    BMCWEB_LOG_INFO("Response:  {} {}", req.url().encoded_path(),
                    res.resultInt());
    addSecurityHeaders(req, res);

    authentication::cleanupTempSession(req);

    res.setHashAndHandleNotModified();
    std::visit(
        [&req, &res](auto&& bodyResp) {
        completeResponseFieldsImpl(req, res, bodyResp);
        },
        res.genericResponse.value());
}

} // namespace crow
