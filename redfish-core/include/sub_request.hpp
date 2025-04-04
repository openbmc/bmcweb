// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "http_request.hpp"

#include <boost/beast/http/verb.hpp>
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

    std::string_view url() const
    {
        return url_;
    }

    boost::beast::http::verb method() const
    {
        return method_;
    }

  private:
    std::string url_;
    boost::beast::http::verb method_;
};

} // namespace redfish 