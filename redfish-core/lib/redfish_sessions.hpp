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

#include "error_messages.hpp"
#include "node.hpp"
#include "persistent_data_middleware.hpp"

namespace redfish
{

class SessionCollection;

class Sessions : public Node
{
  public:
    Sessions(CrowApp& app) :
        Node(app, "/redfish/v1/SessionService/Sessions/<str>/", std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_,
             {{"ConfigureManager"}, {"ConfigureSelf"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        // Note that control also reaches here via doPost and doDelete.
        auto session =
            crow::persistent_data::SessionStore::getInstance().getSessionByUid(
                params[0]);

        if (session == nullptr)
        {
            messages::resourceNotFound(res, "Session", params[0]);
            res.end();
            return;
        }

        res.jsonValue["Id"] = session->uniqueId;
        res.jsonValue["UserName"] = session->username;
        res.jsonValue["@odata.id"] =
            "/redfish/v1/SessionService/Sessions/" + session->uniqueId;
        res.jsonValue["@odata.type"] = "#Session.v1_0_2.Session";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#Session.Session";
        res.jsonValue["Name"] = "User Session";
        res.jsonValue["Description"] = "Manager User Session";

        res.end();
    }

    void doDelete(crow::Response& res, const crow::Request& req,
                  const std::vector<std::string>& params) override
    {
        // Need only 1 param which should be id of session to be deleted
        if (params.size() != 1)
        {
            // This should be handled by crow and never happen
            BMCWEB_LOG_ERROR << "Session DELETE has been called with invalid "
                                "number of params";

            messages::generalError(res);
            res.end();
            return;
        }

        auto session =
            crow::persistent_data::SessionStore::getInstance().getSessionByUid(
                params[0]);

        if (session == nullptr)
        {
            messages::resourceNotFound(res, "Session", params[0]);
            res.end();
            return;
        }

        // Perform a proper ConfigureSelf authority check.  If a
        // session is being used to DELETE some other user's session,
        // then the ConfigureSelf privilege does not apply.  In that
        // case, perform the authority check again without the user's
        // ConfigureSelf privilege.
        if (session->username != req.session->username)
        {
            if (!isAllowedWithoutConfigureSelf(req))
            {
                BMCWEB_LOG_WARNING << "DELETE Session denied access";
                messages::insufficientPrivilege(res);
                res.end();
                return;
            }
        }

        // DELETE should return representation of object that will be removed
        doGet(res, req, params);

        crow::persistent_data::SessionStore::getInstance().removeSession(
            session);
    }

    /**
     * This allows SessionCollection to reuse this class' doGet method, to
     * maintain consistency of returned data, as Collection's doPost should
     * return data for created member which should match member's doGet result
     * in 100%
     */
    friend SessionCollection;
};

class SessionCollection : public Node
{
  public:
    SessionCollection(CrowApp& app) :
        Node(app, "/redfish/v1/SessionService/Sessions/"), memberSession(app)
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        std::vector<const std::string*> sessionIds =
            crow::persistent_data::SessionStore::getInstance().getUniqueIds(
                false, crow::persistent_data::PersistenceType::TIMEOUT);

        res.jsonValue["Members@odata.count"] = sessionIds.size();
        res.jsonValue["Members"] = nlohmann::json::array();
        for (const std::string* uid : sessionIds)
        {
            res.jsonValue["Members"].push_back(
                {{"@odata.id", "/redfish/v1/SessionService/Sessions/" + *uid}});
        }
        res.jsonValue["Members@odata.count"] = sessionIds.size();
        res.jsonValue["@odata.type"] = "#SessionCollection.SessionCollection";
        res.jsonValue["@odata.id"] = "/redfish/v1/SessionService/Sessions/";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#SessionCollection.SessionCollection";
        res.jsonValue["Name"] = "Session Collection";
        res.jsonValue["Description"] = "Session Collection";
        res.end();
    }

    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>& params) override
    {
        std::string username;
        std::string password;
        if (!json_util::readJson(req, res, "UserName", username, "Password",
                                 password))
        {
            res.end();
            return;
        }

        if (password.empty() || username.empty() ||
            res.result() != boost::beast::http::status::ok)
        {
            if (username.empty())
            {
                messages::propertyMissing(res, "UserName");
            }

            if (password.empty())
            {
                messages::propertyMissing(res, "Password");
            }
            res.end();

            return;
        }

        if (pamAuthenticateUser(username, password) != PAM_SUCCESS)
        {
            messages::resourceAtUriUnauthorized(res, std::string(req.url),
                                                "Invalid username or password");
            res.end();

            return;
        }

        // User is authenticated - create session
        std::shared_ptr<crow::persistent_data::UserSession> session =
            crow::persistent_data::SessionStore::getInstance()
                .generateUserSession(username);
        res.addHeader("X-Auth-Token", session->sessionToken);
        res.addHeader("Location", "/redfish/v1/SessionService/Sessions/" +
                                      session->uniqueId);
        res.result(boost::beast::http::status::created);
        memberSession.doGet(res, req, {session->uniqueId});
    }

    /**
     * Member session to ensure consistency between collection's doPost and
     * member's doGet, as they should return 100% matching data
     */
    Sessions memberSession;
};

class SessionService : public Node
{
  public:
    SessionService(CrowApp& app) : Node(app, "/redfish/v1/SessionService/")
    {

        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        res.jsonValue["@odata.type"] = "#SessionService.v1_0_2.SessionService";
        res.jsonValue["@odata.id"] = "/redfish/v1/SessionService/";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#SessionService.SessionService";
        res.jsonValue["Name"] = "Session Service";
        res.jsonValue["Id"] = "SessionService";
        res.jsonValue["Description"] = "Session Service";
        res.jsonValue["SessionTimeout"] =
            crow::persistent_data::SessionStore::getInstance()
                .getTimeoutInSeconds();
        res.jsonValue["ServiceEnabled"] = true;

        res.jsonValue["Sessions"] = {
            {"@odata.id", "/redfish/v1/SessionService/Sessions"}};

        res.end();
    }
};

} // namespace redfish
