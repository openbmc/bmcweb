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

inline void nlohmannToBoostJson(const nlohmann::json& jsonIn,
                                boost::json::value& value)
{
    const nlohmann::json::object_t* objIn =
        jsonIn.get_ptr<const nlohmann::json::object_t*>();
    if (objIn != nullptr)
    {
        boost::json::object& jsonOut = value.emplace_object();
        for (const auto& [key, mappedValue] : *objIn)
        {
            std::pair<boost::json::object::iterator, bool> it =
                jsonOut.emplace(std::string_view(key), nullptr);
            nlohmannToBoostJson(mappedValue, it.first->value());
        }
    }
    const nlohmann::json::array_t* arrIn =
        jsonIn.get_ptr<const nlohmann::json::array_t*>();
    if (arrIn != nullptr)
    {
        boost::json::array& jsonArrOut = value.emplace_array();
        jsonArrOut.reserve(arrIn->size());
        for (const auto& arrValue : *objIn)
        {
            boost::json::value& val = jsonArrOut.emplace_back(nullptr);
            nlohmannToBoostJson(arrValue, val);
        }
    }
    const uint64_t* uint64In = jsonIn.get_ptr<const uint64_t*>();
    if (uint64In != nullptr)
    {
        value.emplace_uint64() = *uint64In;
    }
    const int64_t* int64In = jsonIn.get_ptr<const int64_t*>();
    if (int64In != nullptr)
    {
        value.emplace_int64() = *int64In;
    }
    const double* doubleIn = jsonIn.get_ptr<const double*>();
    if (doubleIn != nullptr)
    {
        value.emplace_double() = *doubleIn;
    }
    const std::string* strIn = jsonIn.get_ptr<const std::string*>();
    if (strIn != nullptr)
    {
        value.emplace_string() = *strIn;
    }
    const bool* boolIn = jsonIn.get_ptr<const bool*>();
    if (boolIn != nullptr)
    {
        value.emplace_bool() = *boolIn;
    }
}

inline void completeResponseFields(std::string_view accepts, Response& res)
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
            nlohmannToBoostJson(res.jsonValue, res.response.body().jsonValue2);
        }
        res.jsonValue.clear();
    }
}
} // namespace crow
