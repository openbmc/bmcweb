// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include "http_request.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/url/url.hpp>

namespace redfish
{
// Drop HTTP/2 pseudo-headers (those whose names start with ':', e.g.
// :authority) Drop any incoming Host and x-auth-token headers
inline void filterHeaders(const std::shared_ptr<crow::Request>& localReq)
{
    // Only allow Host,Accept and ContentType from the request's header
    auto fields = localReq->fields();
    for (const auto& it : fields)
    {
        auto h = it.name();
        auto hString = it.name_string();

        // If header name contains ':' drop it as it is HTTP2 pseudo-header
        if (hString.find(':') != std::string::npos)
        {
            localReq->clearHeader(hString);
        }
        if (hString == "x-auth-token" || hString == "X-Auth-Token" ||
            hString == "x-Auth-Token" || hString == "X-AUTH-TOKEN")
        {
            localReq->clearHeader(hString);
        }

        if (h == boost::beast::http::field::content_type ||
            h == boost::beast::http::field::host)
        {
            continue;
        }
        localReq->clearHeader(h);
    }
    localReq->addHeader(boost::beast::http::field::accept,
                        "application/json, application/octet-stream");
}
} // namespace redfish
