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

inline void getSessionInfo(
    const std::shared_ptr<AsyncResp>& sessionAsyncResp,
    const std::shared_ptr<std::vector<std::string>>& ipmiSessions)
{
    BMCWEB_LOG_DEBUG << "Get available Sessions info.";

    sessionAsyncResp->res.jsonValue["Members"] = nlohmann::json::array();
    for (const std::string& sid : *ipmiSessions)
    {
        if (sid.length() != 0)
        {
            sessionAsyncResp->res.jsonValue["Members"].push_back(
                {{"@odata.id", "/redfish/v1/SessionService/Sessions/" + sid}});
        }
    }

    std::vector<const std::string*> sessionIds =
        persistent_data::SessionStore::getInstance().getUniqueIds(
            false, persistent_data::PersistenceType::TIMEOUT);
    for (const std::string* uid : sessionIds)
    {
        if ((*uid).length() != 0)
        {
            sessionAsyncResp->res.jsonValue["Members"].push_back(
                {{"@odata.id", "/redfish/v1/SessionService/Sessions/" + *uid}});
        }
    }
    sessionAsyncResp->res.jsonValue["Members@odata.count"] =
        sessionAsyncResp->res.jsonValue["Members"].size();
    sessionAsyncResp->res.jsonValue["@odata.type"] =
        "#SessionCollection.SessionCollection";
    sessionAsyncResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/SessionService/Sessions/";
    sessionAsyncResp->res.jsonValue["Name"] = "Session Collection";
    sessionAsyncResp->res.jsonValue["Description"] = "Session Collection";
    sessionAsyncResp->res.end();
}

inline void getSessionHandle(const std::shared_ptr<AsyncResp>& sessionAsyncResp)
{
    BMCWEB_LOG_DEBUG << "Get available Ipmi Sessions info.";

    std::array<std::string, 1> interfaces = {
        "xyz.openbmc_project.Ipmi.SessionInfo"};
    std::vector<std::string> ipmiSessions;

    auto sessionSharedPtr =
        std::make_shared<std::vector<std::string>>(ipmiSessions);

    crow::connections::systemBus->async_method_call(
        [sessionAsyncResp, sessionSharedPtr](const boost::system::error_code ec,
                                             const GetSubTreeType& subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG
                    << "Error in querying GetSubTree with Object Mapper. "
                    << ec;
                messages::internalError(sessionAsyncResp->res);
                return;
            }
            if (subtree.size() == 0)
            {
                BMCWEB_LOG_DEBUG << "Can't find  Session Info Attributes!";
                getSessionInfo(sessionAsyncResp, sessionSharedPtr);
            }

            size_t subtreeEnd = 0;
            auto subtreeEndPtr = std::make_shared<size_t>(subtreeEnd);
            for (const std::pair<std::string,
                                 std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>&
                     object : subtree)
            {
                if (object.first.empty() || object.second.empty())
                {
                    BMCWEB_LOG_DEBUG << "Session Info Attributes mapper error!";
                    continue;
                }

                const std::string& objPath = object.first;
                for (const std::pair<std::string, std::vector<std::string>>&
                         objData : object.second)
                {
                    const std::string& objectServiceName = objData.first;
                    if (objectServiceName.empty())
                    {
                        BMCWEB_LOG_DEBUG
                            << "Session Info Attributes mapper error!";
                        continue;
                    }

                    crow::connections::systemBus->async_method_call(
                        [sessionAsyncResp, sessionSharedPtr, objectServiceName,
                         objPath, subtreeEndPtr,
                         subtree](const boost::system::error_code ec,
                                  std::variant<uint8_t> state) {
                            if (ec)
                            {
                                BMCWEB_LOG_DEBUG << "Error in querying Session "
                                                    "Info State property "
                                                 << ec;
                                messages::internalError(sessionAsyncResp->res);
                                return;
                            }
                            (*subtreeEndPtr)++;
                            const uint8_t* sessionState =
                                std::get_if<uint8_t>(&state);
                            if (sessionState != nullptr && *sessionState != 0)
                            {
                                size_t lastSlash = objPath.find_last_of('/');
                                std::string sessionName =
                                    objPath.substr(lastSlash + 1);
                                sessionSharedPtr->push_back(sessionName);
                            }
                            if (*subtreeEndPtr == subtree.size())
                            {
                                getSessionInfo(sessionAsyncResp,
                                               sessionSharedPtr);
                            }
                        },
                        objectServiceName, objPath,
                        "org.freedesktop.DBus.Properties", "Get",
                        "xyz.openbmc_project.Ipmi.SessionInfo", "State");
                }
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", "/", 0, interfaces);
}

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
    void doGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const crow::Request&,
               const std::vector<std::string>& params) override
    {
        // Note that control also reaches here via doPost and doDelete.
        auto session =
            persistent_data::SessionStore::getInstance().getSessionByUid(
                params[0]);

        if (session == nullptr)
        {
            messages::resourceNotFound(asyncResp->res, "Session", params[0]);
            return;
        }

        asyncResp->res.jsonValue["Id"] = session->uniqueId;
        asyncResp->res.jsonValue["UserName"] = session->username;
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/SessionService/Sessions/" + session->uniqueId;
        asyncResp->res.jsonValue["@odata.type"] = "#Session.v1_3_0.Session";
        asyncResp->res.jsonValue["Name"] = "User Session";
        asyncResp->res.jsonValue["Description"] = "Manager User Session";
        asyncResp->res.jsonValue["ClientOriginIPAddress"] = session->clientIp;
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
        asyncResp->res.jsonValue["Oem"]["OpenBMC"]["@odata.type"] =
            "#OemSession.v1_0_0.Session";
        asyncResp->res.jsonValue["Oem"]["OpenBMC"]["ClientID"] =
            session->clientId;
#endif
    }

    void doDelete(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                  const crow::Request& req,
                  const std::vector<std::string>& params) override
    {
        // Need only 1 param which should be id of session to be deleted
        if (params.size() != 1)
        {
            // This should be handled by crow and never happen
            BMCWEB_LOG_ERROR << "Session DELETE has been called with invalid "
                                "number of params";

            messages::generalError(asyncResp->res);
            return;
        }

        auto session =
            persistent_data::SessionStore::getInstance().getSessionByUid(
                params[0]);

        if (session == nullptr)
        {
            messages::resourceNotFound(asyncResp->res, "Session", params[0]);
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
                messages::insufficientPrivilege(asyncResp->res);
                return;
            }
        }

        // DELETE should return representation of object that will be removed
        doGet(asyncResp, req, params);

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
    void doGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const crow::Request&, const std::vector<std::string>&) override
    {
        std::vector<const std::string*> sessionIds =
            persistent_data::SessionStore::getInstance().getUniqueIds(
                false, persistent_data::PersistenceType::TIMEOUT);

        asyncResp->res.jsonValue["Members@odata.count"] = sessionIds.size();
        asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
        for (const std::string* uid : sessionIds)
        {
            asyncResp->res.jsonValue["Members"].push_back(
                {{"@odata.id", "/redfish/v1/SessionService/Sessions/" + *uid}});
        }
        asyncResp->res.jsonValue["Members@odata.count"] = sessionIds.size();
        asyncResp->res.jsonValue["@odata.type"] =
            "#SessionCollection.SessionCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/SessionService/Sessions/";
        asyncResp->res.jsonValue["Name"] = "Session Collection";
        asyncResp->res.jsonValue["Description"] = "Session Collection";
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        getSessionHandle(asyncResp);
    }

    void doPost(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const crow::Request& req,
                const std::vector<std::string>&) override
    {
        std::string username;
        std::string password;
        std::optional<nlohmann::json> oemObject;
        std::string clientId;
        if (!json_util::readJson(req, asyncResp->res, "UserName", username,
                                 "Password", password, "Oem", oemObject))
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
            messages::resourceAtUriUnauthorized(asyncResp->res,
                                                std::string(req.url),
                                                "Invalid username or password");
            return;
        }
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
        if (oemObject)
        {
            std::optional<nlohmann::json> bmcOem;
            if (!json_util::readJson(*oemObject, asyncResp->res, "OpenBMC",
                                     bmcOem))
            {
                return;
            }
            if (!json_util::readJson(*bmcOem, asyncResp->res, "ClientID",
                                     clientId))
            {
                BMCWEB_LOG_ERROR << "Could not read ClientId";
                return;
            }
        }
#endif

        // User is authenticated - create session
        std::shared_ptr<persistent_data::UserSession> session =
            persistent_data::SessionStore::getInstance().generateUserSession(
                username, req.ipAddress.to_string(), clientId,
                persistent_data::PersistenceType::TIMEOUT, isConfigureSelfOnly);
        asyncResp->res.addHeader("X-Auth-Token", session->sessionToken);
        asyncResp->res.addHeader("Location",
                                 "/redfish/v1/SessionService/Sessions/" +
                                     session->uniqueId);
        asyncResp->res.result(boost::beast::http::status::created);
        if (session->isConfigureSelfOnly)
        {
            messages::passwordChangeRequired(
                asyncResp->res,
                "/redfish/v1/AccountService/Accounts/" + session->username);
        }
        memberSession.doGet(asyncResp, req, {session->uniqueId});
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
    void doGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const crow::Request&, const std::vector<std::string>&) override
    {
        asyncResp->res.jsonValue["@odata.type"] =
            "#SessionService.v1_0_2.SessionService";
        asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/SessionService/";
        asyncResp->res.jsonValue["Name"] = "Session Service";
        asyncResp->res.jsonValue["Id"] = "SessionService";
        asyncResp->res.jsonValue["Description"] = "Session Service";
        asyncResp->res.jsonValue["SessionTimeout"] =
            persistent_data::SessionStore::getInstance().getTimeoutInSeconds();
        asyncResp->res.jsonValue["ServiceEnabled"] = true;

        asyncResp->res.jsonValue["Sessions"] = {
            {"@odata.id", "/redfish/v1/SessionService/Sessions"}};
    }

    void doPatch(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                 const crow::Request& req,
                 const std::vector<std::string>&) override
    {
        std::optional<int64_t> sessionTimeout;
        if (!json_util::readJson(req, asyncResp->res, "SessionTimeout",
                                 sessionTimeout))
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
                    asyncResp->res, std::to_string(*sessionTimeout),
                    "SessionTimeOut");
            }
        }
    }
};

} // namespace redfish
