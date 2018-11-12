/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#pragma once

#include "privileges.hpp"
#include "token_authorization_middleware.hpp"
#include "webserver_common.hpp"

#include <error_messages.hpp>

#include "crow.h"

namespace redfish
{

/**
 * AsyncResp
 * Gathers data needed for response processing after async calls are done
 */
class AsyncResp
{
  public:
    AsyncResp(crow::Response& response) : res(response)
    {
    }

    ~AsyncResp()
    {
        res.end();
    }

    crow::Response& res;
};

/**
 * @brief  Abstract class used for implementing Redfish nodes.
 *
 */
class Node
{
  public:
    template <typename... Params>
    Node(CrowApp& app, std::string&& entityUrl, Params... params)
    {
        app.routeDynamic(entityUrl.c_str())
            .methods("GET"_method, "PATCH"_method, "POST"_method,
                     "DELETE"_method)([&](const crow::Request& req,
                                          crow::Response& res,
                                          Params... params) {
                std::vector<std::string> paramVec = {params...};
                dispatchRequest(app, req, res, paramVec);
            });
    }

    virtual ~Node() = default;

    OperationMap entityPrivileges;

  protected:
    // Node is designed to be an abstract class, so doGet is pure virtual
    virtual void doGet(crow::Response& res, const crow::Request& req,
                       const std::vector<std::string>& params)
    {
        res.result(boost::beast::http::status::method_not_allowed);
        res.end();
    }

    virtual void doPatch(crow::Response& res, const crow::Request& req,
                         const std::vector<std::string>& params)
    {
        res.result(boost::beast::http::status::method_not_allowed);
        res.end();
    }

    virtual void doPost(crow::Response& res, const crow::Request& req,
                        const std::vector<std::string>& params)
    {
        res.result(boost::beast::http::status::method_not_allowed);
        res.end();
    }

    virtual void doDelete(crow::Response& res, const crow::Request& req,
                          const std::vector<std::string>& params)
    {
        res.result(boost::beast::http::status::method_not_allowed);
        res.end();
    }

  private:
    void dispatchRequest(CrowApp& app, const crow::Request& req,
                         crow::Response& res,
                         const std::vector<std::string>& params)
    {
        auto ctx =
            app.template getContext<crow::token_authorization::Middleware>(req);

        if (!isMethodAllowedForUser(req.method(), entityPrivileges,
                                    ctx.session->username))
        {
            res.result(boost::beast::http::status::method_not_allowed);
            res.end();
            return;
        }

        switch (req.method())
        {
            case "GET"_method:
                doGet(res, req, params);
                break;

            case "PATCH"_method:
                doPatch(res, req, params);
                break;

            case "POST"_method:
                doPost(res, req, params);
                break;

            case "DELETE"_method:
                doDelete(res, req, params);
                break;

            default:
                res.result(boost::beast::http::status::not_found);
                res.end();
        }
        return;
    }
};

} // namespace redfish
