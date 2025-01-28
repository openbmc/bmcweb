// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "http_request.hpp"

#include <boost/beast/http/verb.hpp>
#include <nlohmann/json.hpp>

#include <string>
#include <string_view>

namespace redfish
{

class SubRequest
{
  public:
    explicit SubRequest(const crow::Request& req) :
        url_(req.url().encoded_path()), method_(req.method())
    {}

    SubRequest(const crow::Request& req,
               nlohmann::json::object_t&& jsonPayload) :
        url_(req.url().encoded_path()), method_(req.method()),
        payload_(std::move(jsonPayload))
    {}

    SubRequest(const SubRequest& req, nlohmann::json::object_t&& newPayload) :
        url_(req.url()), method_(req.method()), payload_(std::move(newPayload))
    {}

    std::string_view url() const
    {
        return url_;
    }

    boost::beast::http::verb method() const
    {
        return method_;
    }

    const nlohmann::json::object_t& payload() const
    {
        return payload_;
    }

  private:
    std::string url_;
    boost::beast::http::verb method_;
    nlohmann::json::object_t payload_;
};

} // namespace redfish
