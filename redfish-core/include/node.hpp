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

#include "http_request.hpp"
#include "http_response.hpp"
#include "privileges.hpp"

#include <error_messages.hpp>

#include <vector>

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
    {}

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
    Node(App& app, std::string&& entityUrl, [[maybe_unused]] Params... paramsIn)
    {
        crow::DynamicRule& get = app.routeDynamic(entityUrl.c_str());
        getRule = &get;
        get.methods(boost::beast::http::verb::get)(
            [this](const crow::Request& req, crow::Response& res,
                   Params... params) {
                std::vector<std::string> paramVec = {params...};
                doGet(res, req, paramVec);
            });

        crow::DynamicRule& patch = app.routeDynamic(entityUrl.c_str());
        patchRule = &patch;
        patch.methods(boost::beast::http::verb::patch)(
            [this](const crow::Request& req, crow::Response& res,
                   Params... params) {
                std::vector<std::string> paramVec = {params...};
                doPatch(res, req, paramVec);
            });

        crow::DynamicRule& post = app.routeDynamic(entityUrl.c_str());
        postRule = &post;
        post.methods(boost::beast::http::verb::post)(
            [this](const crow::Request& req, crow::Response& res,
                   Params... params) {
                std::vector<std::string> paramVec = {params...};
                doPost(res, req, paramVec);
            });

        crow::DynamicRule& put = app.routeDynamic(entityUrl.c_str());
        putRule = &put;
        put.methods(boost::beast::http::verb::put)(
            [this](const crow::Request& req, crow::Response& res,
                   Params... params) {
                std::vector<std::string> paramVec = {params...};
                doPut(res, req, paramVec);
            });

        crow::DynamicRule& deleteR = app.routeDynamic(entityUrl.c_str());
        deleteRule = &deleteR;
        deleteR.methods(boost::beast::http::verb::delete_)(
            [this](const crow::Request& req, crow::Response& res,
                   Params... params) {
                std::vector<std::string> paramVec = {params...};
                doDelete(res, req, paramVec);
            });
    }

    void initPrivileges()
    {
        auto it = entityPrivileges.find(boost::beast::http::verb::get);
        if (it != entityPrivileges.end())
        {
            if (getRule != nullptr)
            {
                getRule->privileges(it->second);
            }
        }
        it = entityPrivileges.find(boost::beast::http::verb::post);
        if (it != entityPrivileges.end())
        {
            if (postRule != nullptr)
            {
                postRule->privileges(it->second);
            }
        }
        it = entityPrivileges.find(boost::beast::http::verb::patch);
        if (it != entityPrivileges.end())
        {
            if (patchRule != nullptr)
            {
                patchRule->privileges(it->second);
            }
        }
        it = entityPrivileges.find(boost::beast::http::verb::put);
        if (it != entityPrivileges.end())
        {
            if (putRule != nullptr)
            {
                putRule->privileges(it->second);
            }
        }
        it = entityPrivileges.find(boost::beast::http::verb::delete_);
        if (it != entityPrivileges.end())
        {
            if (deleteRule != nullptr)
            {
                deleteRule->privileges(it->second);
            }
        }
    }

    virtual ~Node() = default;

    OperationMap entityPrivileges;

    crow::DynamicRule* getRule = nullptr;
    crow::DynamicRule* postRule = nullptr;
    crow::DynamicRule* patchRule = nullptr;
    crow::DynamicRule* putRule = nullptr;
    crow::DynamicRule* deleteRule = nullptr;

  protected:
    // Node is designed to be an abstract class, so doGet is pure virtual
    virtual void doGet(crow::Response& res, const crow::Request&,
                       const std::vector<std::string>&)
    {
        res.result(boost::beast::http::status::method_not_allowed);
        res.end();
    }

    virtual void doPatch(crow::Response& res, const crow::Request&,
                         const std::vector<std::string>&)
    {
        res.result(boost::beast::http::status::method_not_allowed);
        res.end();
    }

    virtual void doPost(crow::Response& res, const crow::Request&,
                        const std::vector<std::string>&)
    {
        res.result(boost::beast::http::status::method_not_allowed);
        res.end();
    }

    virtual void doPut(crow::Response& res, const crow::Request&,
                       const std::vector<std::string>&)
    {
        res.result(boost::beast::http::status::method_not_allowed);
        res.end();
    }

    virtual void doDelete(crow::Response& res, const crow::Request&,
                          const std::vector<std::string>&)
    {
        res.result(boost::beast::http::status::method_not_allowed);
        res.end();
    }

    /* @brief Would the operation be allowed if the user did not have the
     * ConfigureSelf Privilege?  Also honors session.isConfigureSelfOnly.
     *
     * @param req      the request
     *
     * @returns        True if allowed, false otherwise
     */
    inline bool isAllowedWithoutConfigureSelf(const crow::Request& req)
    {
        const std::string& userRole = req.userRole;
        BMCWEB_LOG_DEBUG << "isAllowedWithoutConfigureSelf for the role "
                         << req.userRole;
        Privileges effectiveUserPrivileges;
        if (req.session && req.session->isConfigureSelfOnly)
        {
            // The session has no privileges because it is limited to
            // configureSelfOnly and we are disregarding that privilege.
            // Note that some operations do not require any privilege.
        }
        else
        {
            effectiveUserPrivileges = redfish::getUserPrivileges(userRole);
            effectiveUserPrivileges.resetSinglePrivilege("ConfigureSelf");
        }
        const auto& requiredPrivilegesIt = entityPrivileges.find(req.method());
        return (requiredPrivilegesIt != entityPrivileges.end()) &&
               isOperationAllowedWithPrivileges(requiredPrivilegesIt->second,
                                                effectiveUserPrivileges);
    }
};

} // namespace redfish
