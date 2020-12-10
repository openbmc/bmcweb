#pragma once
#include "node.hpp"

#include <systemd/sd-journal.h>

#include <boost/beast/core/span.hpp>
// #include <error_messages.hpp>

#include <string_view>
#include <variant>

namespace redfish
{
namespace query_param
{
enum class QueryParamType
{
    expand,
    only,
    filter,
    select,
    skip,
    top,
    NOPARAM
};

QueryParamType getQueryParam(const crow::Request& req)
{
    boost::urls::url_view::params_type::iterator itOnly =
        req.urlParams.find("only");
    if (itOnly != req.urlParams.end())
    {
        return QueryParamType::only;
    }

    // TODO: Retrieve other parameters.

    return QueryParamType::NOPARAM;
}

void executeQueryParam(QueryParamType queryParam, crow::Response& res)
{
    if (queryParam == redfish::query_param::QueryParamType::only)
    {
        if (res.jsonValue["Members@odata.count"] == 1)
        {
            std::string url = res.jsonValue["Members"][0]["@odata.id"];
            std::string_view URL{url};

            res.result(boost::beast::http::status::temporary_redirect);
            res.addHeader("Location", URL);
        }
        else
        {
            std::cerr << "The member counts is "
                      << res.jsonValue["Members@odata.count"]
                      << " not meet the 'only' requirement." << std::endl;
        }

        // res.end();
    }
    return;
    // TODO: other paramter
}

} // namespace query_param

} // namespace redfish