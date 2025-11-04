// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include "http_request.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/url/url.hpp>

#include <memory>
namespace redfish
{
// Drop any incoming x-auth-token headers and keep Host and Content-Type. Set
// Accept.
inline crow::Request createNewRequest(const crow::Request& localReq,
                                      std::error_code& ec)
{
    // Note, this is an expensive copy.  It ideally shouldn't be done, but no
    // option at this point.
    crow::Request req(localReq.body(), ec);
    if (ec)
    {
        BMCWEB_LOG_ERROR("Failed to set body.  Continuing");
    }

    for (const auto& field : req.fields())
    {
        // Drop any incoming x-auth-token headers and keep Host and
        // Content-Type. Set Accept.
        auto headerName = field.name();
        if (headerName == boost::beast::http::field::content_type ||
            headerName == boost::beast::http::field::host)
        {
            req.addHeader(headerName, field.value());
        }
    }
    req.addHeader(boost::beast::http::field::accept,
                  "application/json, application/octet-stream");
    return req;
}
} // namespace redfish
