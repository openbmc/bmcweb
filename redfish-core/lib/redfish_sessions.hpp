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

#include "app.hpp"
#include "error_messages.hpp"
#include "http/utility.hpp"
#include "persistent_data.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/json_utils.hpp"

namespace redfish
{

inline void fillSessionObject(crow::Response& res,
                              const persistent_data::UserSession& session)
{
    res.jsonValue["Id"] = session.uniqueId;
    res.jsonValue["UserName"] = session.username;
    res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "SessionService", "Sessions", session.uniqueId);
    res.jsonValue["@odata.type"] = "#Session.v1_5_0.Session";
    res.jsonValue["Name"] = "User Session";
    res.jsonValue["Description"] = "Manager User Session";
    res.jsonValue["ClientOriginIPAddress"] = session.clientIp;
    if (session.clientId)
    {
        res.jsonValue["Context"] = *session.clientId;
    }
}

inline void
    handleSessionHead(crow::App& app, const crow::Request& req,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& /*sessionId*/)
{

    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/Session/Session.json>; rel=describedby");
}

inline void
    handleSessionGet(crow::App& app, const crow::Request& req,
                     const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& sessionId)
{
    handleSessionHead(app, req, asyncResp, sessionId);

    // Note that control also reaches here via doPost and doDelete.
    auto session =
        persistent_data::SessionStore::getInstance().getSessionByUid(sessionId);

    if (session == nullptr)
    {
        messages::resourceNotFound(asyncResp->res, "Session", sessionId);
        return;
    }

    fillSessionObject(asyncResp->res, *session);
}

inline void
    handleSessionDelete(crow::App& app, const crow::Request& req,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& sessionId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    auto session =
        persistent_data::SessionStore::getInstance().getSessionByUid(sessionId);

    if (session == nullptr)
    {
        messages::resourceNotFound(asyncResp->res, "Session", sessionId);
        return;
    }

    // Perform a proper ConfigureSelf authority check.  If a
    // session is being used to DELETE some other user's session,
    // then the ConfigureSelf privilege does not apply.  In that
    // case, perform the authority check again without the user's
    // ConfigureSelf privilege.
    if (req.session != nullptr && !session->username.empty() &&
        session->username != req.session->username)
    {
        Privileges effectiveUserPrivileges =
            redfish::getUserPrivileges(req.userRole);

        if (!effectiveUserPrivileges.isSupersetOf({"ConfigureUsers"}))
        {
            messages::insufficientPrivilege(asyncResp->res);
            return;
        }
    }

    persistent_data::SessionStore::getInstance().removeSession(session);
    messages::success(asyncResp->res);
}

inline nlohmann::json getSessionCollectionMembers()
{
    std::vector<const std::string*> sessionIds =
        persistent_data::SessionStore::getInstance().getUniqueIds(
            false, persistent_data::PersistenceType::TIMEOUT);
    nlohmann::json ret = nlohmann::json::array();
    for (const std::string* uid : sessionIds)
    {
        nlohmann::json::object_t session;
        session["@odata.id"] = crow::utility::urlFromPieces(
            "redfish", "v1", "SessionService", "Sessions", *uid);
        ret.push_back(std::move(session));
    }
    return ret;
}

inline void handleSessionCollectionHead(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{

    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/SessionCollection.json>; rel=describedby");
}

inline void handleSessionCollectionGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    handleSessionCollectionHead(app, req, asyncResp);
    asyncResp->res.jsonValue["Members"] = getSessionCollectionMembers();
    asyncResp->res.jsonValue["Members@odata.count"] =
        asyncResp->res.jsonValue["Members"].size();
    asyncResp->res.jsonValue["@odata.type"] =
        "#SessionCollection.SessionCollection";
    asyncResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/SessionService/Sessions/";
    asyncResp->res.jsonValue["Name"] = "Session Collection";
    asyncResp->res.jsonValue["Description"] = "Session Collection";
}

inline void handleSessionCollectionMembersGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.jsonValue = getSessionCollectionMembers();
}

inline void handleSessionCollectionPost(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    std::string username;
    std::string password;
    std::optional<std::string> clientId;
    if (!json_util::readJsonPatch(req, asyncResp->res, "UserName", username,
                                  "Password", password, "Context", clientId))
    {
        return;
    }

    if (password.empty() || username.empty() ||
        asyncResp->res.result() != boost::beast::http::status::ok)
    {
        if (username.empty())
        {
            messages::propertyMissing(asyncResp->res, "UserName");
        }

        if (password.empty())
        {
            messages::propertyMissing(asyncResp->res, "Password");
        }

        return;
    }

    int pamrc = pamAuthenticateUser(username, password);
    bool isConfigureSelfOnly = pamrc == PAM_NEW_AUTHTOK_REQD;
    if ((pamrc != PAM_SUCCESS) && !isConfigureSelfOnly)
    {
        messages::resourceAtUriUnauthorized(asyncResp->res, req.url(),
                                            "Invalid username or password");
        return;
    }

    // User is authenticated - create session
    std::shared_ptr<persistent_data::UserSession> session =
        persistent_data::SessionStore::getInstance().generateUserSession(
            username, req.ipAddress, clientId,
            persistent_data::PersistenceType::TIMEOUT, isConfigureSelfOnly);
    if (session == nullptr)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    asyncResp->res.addHeader("X-Auth-Token", session->sessionToken);
    asyncResp->res.addHeader(
        "Location", "/redfish/v1/SessionService/Sessions/" + session->uniqueId);
    asyncResp->res.result(boost::beast::http::status::created);
    if (session->isConfigureSelfOnly)
    {
        messages::passwordChangeRequired(
            asyncResp->res,
            crow::utility::urlFromPieces("redfish", "v1", "AccountService",
                                         "Accounts", session->username));
    }

    fillSessionObject(asyncResp->res, *session);
}
inline void handleSessionServiceHead(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{

    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/SessionService/SessionService.json>; rel=describedby");
}
inline void
    handleSessionServiceGet(crow::App& app, const crow::Request& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)

{
    handleSessionServiceHead(app, req, asyncResp);
    asyncResp->res.jsonValue["@odata.type"] =
        "#SessionService.v1_0_2.SessionService";
    asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/SessionService/";
    asyncResp->res.jsonValue["Name"] = "Session Service";
    asyncResp->res.jsonValue["Id"] = "SessionService";
    asyncResp->res.jsonValue["Description"] = "Session Service";
    asyncResp->res.jsonValue["SessionTimeout"] =
        persistent_data::SessionStore::getInstance().getTimeoutInSeconds();
    asyncResp->res.jsonValue["ServiceEnabled"] = true;

    asyncResp->res.jsonValue["Sessions"]["@odata.id"] =
        "/redfish/v1/SessionService/Sessions";
}

inline void handleSessionServicePatch(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    std::optional<int64_t> sessionTimeout;
    if (!json_util::readJsonPatch(req, asyncResp->res, "SessionTimeout",
                                  sessionTimeout))
    {
        return;
    }

    if (sessionTimeout)
    {
        // The mininum & maximum allowed values for session timeout
        // are 30 seconds and 86400 seconds respectively as per the
        // session service schema mentioned at
        // https://redfish.dmtf.org/schemas/v1/SessionService.v1_1_7.json

        if (*sessionTimeout <= 86400 && *sessionTimeout >= 30)
        {
            std::chrono::seconds sessionTimeoutInseconds(*sessionTimeout);
            persistent_data::SessionStore::getInstance().updateSessionTimeout(
                sessionTimeoutInseconds);
            messages::propertyValueModified(asyncResp->res, "SessionTimeOut",
                                            std::to_string(*sessionTimeout));
        }
        else
        {
            messages::propertyValueNotInList(asyncResp->res,
                                             std::to_string(*sessionTimeout),
                                             "SessionTimeOut");
        }
    }
}

inline void requestRoutesSession(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/SessionService/Sessions/<str>/")
        .privileges(redfish::privileges::headSession)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleSessionHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/SessionService/Sessions/<str>/")
        .privileges(redfish::privileges::getSession)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleSessionGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/SessionService/Sessions/<str>/")
        .privileges(redfish::privileges::deleteSession)
        .methods(boost::beast::http::verb::delete_)(
            std::bind_front(handleSessionDelete, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/SessionService/Sessions/")
        .privileges(redfish::privileges::headSessionCollection)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleSessionCollectionHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/SessionService/Sessions/")
        .privileges(redfish::privileges::getSessionCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleSessionCollectionGet, std::ref(app)));

    // Note, the next two routes technically don't match the privilege
    // registry given the way login mechanisms work.  The base privilege
    // registry lists this endpoint as requiring login privilege, but because
    // this is the endpoint responsible for giving the login privilege, and it
    // is itself its own route, it needs to not require Login
    BMCWEB_ROUTE(app, "/redfish/v1/SessionService/Sessions/")
        .privileges({})
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleSessionCollectionPost, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/SessionService/Sessions/Members/")
        .privileges({})
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleSessionCollectionPost, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/SessionService/")
        .privileges(redfish::privileges::headSessionService)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleSessionServiceHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/SessionService/")
        .privileges(redfish::privileges::getSessionService)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleSessionServiceGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/SessionService/")
        .privileges(redfish::privileges::patchSessionService)
        .methods(boost::beast::http::verb::patch)(
            std::bind_front(handleSessionServicePatch, std::ref(app)));
}

} // namespace redfish
