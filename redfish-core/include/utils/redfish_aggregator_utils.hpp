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
inline boost::beast::http::fields createNewHeaders(
    const std::shared_ptr<crow::Request>& localReq)
{
    boost::beast::http::fields newHeaders;
    for (const auto& field : localReq->fields())
    {
        auto headerName = field.name();
        if (headerName == boost::beast::http::field::content_type ||
            headerName == boost::beast::http::field::host)
        {
            newHeaders.insert(headerName, field.value());
        }
    }
    newHeaders.insert(boost::beast::http::field::accept,
                      "application/json, application/octet-stream");
    return newHeaders;
}
} // namespace redfish
