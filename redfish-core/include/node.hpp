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
        auto it = entityPrivileges.find(boost::beast::http::verb::get);
        if (it != entityPrivileges.end())
        {
            app.routeDynamic(entityUrl.c_str())
                .requires(*it)
                .methods("GET"_method)([&](const crow::Request& req,
                                           crow::Response& res,
                                           Params... params) {
                    std::vector<std::string> paramVec = {params...};
                    doGet(res, req, paramVec);
                });
        }
        it = entityPrivileges.find(boost::beast::http::verb::patch);
        if (it != entityPrivileges.end())
        {
            app.routeDynamic(entityUrl.c_str())
                .requires(*it)
                .methods("PATCH"_method)([&](const crow::Request& req,
                                             crow::Response& res,
                                             Params... params) {
                    std::vector<std::string> paramVec = {params...};
                    doPatch(res, req, paramVec);
                });
        }

        it = entityPrivileges.find(boost::beast::http::verb::post);
        if (it != entityPrivileges.end())
        {
            app.routeDynamic(entityUrl.c_str())
                .requires(*it)
                .methods("POST"_method)([&](const crow::Request& req,
                                            crow::Response& res,
                                            Params... params) {
                    std::vector<std::string> paramVec = {params...};
                    doPost(res, req, paramVec);
                });
        }

        it = entityPrivileges.find(boost::beast::http::verb::delete_);
        if (it != entityPrivileges.end())
        {
            app.routeDynamic(entityUrl.c_str())
                .requires(*it)
                .methods("DELETE"_method)([&](const crow::Request& req,
                                              crow::Response& res,
                                              Params... params) {
                    std::vector<std::string> paramVec = {params...};
                    doDelete(res, req, paramVec);
                });
        }
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
};

} // namespace redfish
