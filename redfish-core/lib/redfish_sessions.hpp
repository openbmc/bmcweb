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
#include "persistent_data.hpp"

namespace redfish
{

class SessionCollection;

class Sessions : public Node
{
  public:
    Sessions(App& app) :
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        // Note that control also reaches here via doPost and doDelete.
        auto session =
            persistent_data::SessionStore::getInstance().getSessionByUid(
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
        res.jsonValue["@odata.type"] = "#Session.v1_3_0.Session";
        res.jsonValue["Name"] = "User Session";
        res.jsonValue["Description"] = "Manager User Session";
        res.jsonValue["ClientOriginIPAddress"] = session->clientIp;
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
        res.jsonValue["Oem"]["OpenBMC"]["@odata.type"] =
            "#OemSession.v1_0_0.Session";
        res.jsonValue["Oem"]["OpenBMC"]["ClientID"] = session->clientId;
#endif
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
            persistent_data::SessionStore::getInstance().getSessionByUid(
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

        persistent_data::SessionStore::getInstance().removeSession(session);
    }

    /**
     * This allows SessionCollection to reuse this class' doGet method, to
     * maintain consistency of returned data, as Collection's doPost should
     * return data for created member which should match member's doGet
     * result in 100%
     */
    friend SessionCollection;
};

class SessionCollection : public Node
{
  public:
    SessionCollection(App& app) :
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>&) override
    {
        std::vector<const std::string*> sessionIds =
            persistent_data::SessionStore::getInstance().getUniqueIds(
                false, persistent_data::PersistenceType::TIMEOUT);

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
        res.jsonValue["Name"] = "Session Collection";
        res.jsonValue["Description"] = "Session Collection";
        res.end();
    }

    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>&) override
    {
        std::string username;
        std::string password;
        std::optional<nlohmann::json> oemObject;
        std::string clientId;
        if (!json_util::readJson(req, res, "UserName", username, "Password",
                                 password, "Oem", oemObject))
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

        int pamrc = pamAuthenticateUser(username, password);
        bool isConfigureSelfOnly = pamrc == PAM_NEW_AUTHTOK_REQD;
        if ((pamrc != PAM_SUCCESS) && !isConfigureSelfOnly)
        {
            messages::resourceAtUriUnauthorized(res, std::string(req.url),
                                                "Invalid username or password");
            res.end();

            return;
        }
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
        if (oemObject)
        {
            std::optional<nlohmann::json> bmcOem;
            if (!json_util::readJson(*oemObject, res, "OpenBMC", bmcOem))
            {
                res.end();
                return;
            }
            if (!json_util::readJson(*bmcOem, res, "ClientID", clientId))
            {
                BMCWEB_LOG_ERROR << "Could not read ClientId";
                res.end();
                return;
            }
        }
#endif

        // User is authenticated - create session
        std::shared_ptr<persistent_data::UserSession> session =
            persistent_data::SessionStore::getInstance().generateUserSession(
                username, persistent_data::PersistenceType::TIMEOUT,
                isConfigureSelfOnly, clientId, req.ipAddress.to_string());
        res.addHeader("X-Auth-Token", session->sessionToken);
        res.addHeader("Location", "/redfish/v1/SessionService/Sessions/" +
                                      session->uniqueId);
        res.result(boost::beast::http::status::created);
        if (session->isConfigureSelfOnly)
        {
            messages::passwordChangeRequired(
                res,
                "/redfish/v1/AccountService/Accounts/" + session->username);
        }
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
    SessionService(App& app) : Node(app, "/redfish/v1/SessionService/")
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>&) override
    {
        res.jsonValue["@odata.type"] = "#SessionService.v1_0_2.SessionService";
        res.jsonValue["@odata.id"] = "/redfish/v1/SessionService/";
        res.jsonValue["Name"] = "Session Service";
        res.jsonValue["Id"] = "SessionService";
        res.jsonValue["Description"] = "Session Service";
        res.jsonValue["SessionTimeout"] =
            persistent_data::SessionStore::getInstance().getTimeoutInSeconds();
        res.jsonValue["ServiceEnabled"] = true;

        res.jsonValue["Sessions"] = {
            {"@odata.id", "/redfish/v1/SessionService/Sessions"}};

        res.end();
    }

    void doPatch(crow::Response& res, const crow::Request& req,
                 const std::vector<std::string>&) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        std::optional<int64_t> sessionTimeout;
        if (!json_util::readJson(req, res, "SessionTimeout", sessionTimeout))
        {
            return;
        }

        if (sessionTimeout)
        {
            // The mininum & maximum allowed values for session timeout are 30
            // seconds and 86400 seconds respectively as per the session service
            // schema mentioned at
            // https://redfish.dmtf.org/schemas/v1/SessionService.v1_1_7.json

            if (*sessionTimeout <= 86400 && *sessionTimeout >= 30)
            {
                std::chrono::seconds sessionTimeoutInseconds(*sessionTimeout);
                persistent_data::SessionStore::getInstance()
                    .updateSessionTimeout(sessionTimeoutInseconds);
                messages::propertyValueModified(
                    asyncResp->res, "SessionTimeOut",
                    std::to_string(*sessionTimeout));
            }
            else
            {
                messages::propertyValueNotInList(
                    res, std::to_string(*sessionTimeout), "SessionTimeOut");
            }
        }
    }
};

} // namespace redfish
