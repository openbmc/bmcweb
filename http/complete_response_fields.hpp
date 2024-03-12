#pragma once

#include "authentication.hpp"
#include "boost_formatters.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "http_utility.hpp"
#include "json_html_serializer.hpp"
#include "logging.hpp"
#include "security_headers.hpp"
#include "utils/hex_utils.hpp"

#include <boost/beast/http/message.hpp>
#include <nlohmann/json.hpp>

#include <array>

namespace crow
{

inline void handleEncoding(std::string_view acceptEncoding, Response& res)
{
    using http_helpers::Encoding;
    // If the payload is currently compressed, see if we can avoid
    // decompressing it by sending it to the client directly
    switch (res.response.body().compressionType)
    {
        case bmcweb::CompressionType::Zstd:
        {
            std::array<Encoding, 1> allowedEnc{Encoding::ZSTD};
            Encoding encoding =
                http_helpers::getPreferredEncoding(acceptEncoding, allowedEnc);

            if (encoding == Encoding::ZSTD)
            {
                // If the client supports returning zstd directly, allow that.
                res.response.body().clientCompressionType =
                    bmcweb::CompressionType::Zstd;
            }
        }
        break;
        case bmcweb::CompressionType::Gzip:
        {
            std::array<Encoding, 1> allowedEnc{Encoding::GZIP};
            Encoding encoding =
                http_helpers::getPreferredEncoding(acceptEncoding, allowedEnc);
            if (encoding != Encoding::GZIP)
            {
                BMCWEB_LOG_WARNING(
                    "Unimplemented: Returning gzip payload to client that did not explicitly allow it.");
            }
        }
        break;
        default:
            break;
    }
}

inline void completeResponseFields(
    std::string_view accepts, std::string_view acceptEncoding, Response& res)
{
    BMCWEB_LOG_INFO("Response: {}", res.resultInt());
    addSecurityHeaders(res);

    res.setHashAndHandleNotModified();
    if (res.jsonValue.is_structured())
    {
        using http_helpers::ContentType;
        std::array<ContentType, 3> allowed{ContentType::CBOR, ContentType::JSON,
                                           ContentType::HTML};
        ContentType preferred = getPreferredContentType(accepts, allowed);

        if (preferred == ContentType::HTML)
        {
            json_html_util::prettyPrintJson(res);
        }
        else if (preferred == ContentType::CBOR)
        {
            res.addHeader(boost::beast::http::field::content_type,
                          "application/cbor");
            std::string cbor;
            nlohmann::json::to_cbor(res.jsonValue, cbor);
            res.write(std::move(cbor));
        }
        else
        {
            // Technically preferred could also be NoMatch here, but we'd
            // like to default to something rather than return 400 for
            // backward compatibility.
            res.addHeader(boost::beast::http::field::content_type,
                          "application/json");
            res.write(res.jsonValue.dump(
                2, ' ', true, nlohmann::json::error_handler_t::replace));
        }
    }

    handleEncoding(acceptsEncoding, res);
}
} // namespace crow
