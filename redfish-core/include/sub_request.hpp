// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "http_request.hpp"
#include "parsing.hpp"

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
    {
        // Extract OEM payload if present
        if (req.method() == boost::beast::http::verb::patch ||
            req.method() == boost::beast::http::verb::post)
        {
            nlohmann::json reqJson;
            if (parseRequestAsJson(req, reqJson) != JsonParseResult::Success)
            {
                return;
            }

            auto oemIt = reqJson.find("Oem");
            if (oemIt != reqJson.end())
            {
                const nlohmann::json::object_t* oemObj =
                    oemIt->get_ptr<const nlohmann::json::object_t*>();
                if (oemObj != nullptr && !oemObj->empty())
                {
                    payload_ = *oemObj;
                }
            }
        }
    }

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

    bool needHandling() const
    {
        if (method_ == boost::beast::http::verb::get)
        {
            return true;
        }

        if ((method_ == boost::beast::http::verb::patch ||
             method_ == boost::beast::http::verb::post) &&
            !payload_.empty())
        {
            return true;
        }

        return false;
    }

  private:
    std::string url_;
    boost::beast::http::verb method_;
    nlohmann::json::object_t payload_;
};

} // namespace redfish
