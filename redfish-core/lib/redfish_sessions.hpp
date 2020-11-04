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
    const std::shared_ptr<std::vector<std::string>>& ipmiSessions,
    const std::vector<std::string>& params,
    const std::optional<bool>& sessionDelete)
{
    BMCWEB_LOG_DEBUG << "Get available Sessions info.";

    if (!params.empty())
    {
        // This will support to get session details and to delete valid Redfish
        // session
        bool ipmiSessionsFlag = false;
        if (std::find(ipmiSessions->begin(), ipmiSessions->end(), params[0]) !=
            ipmiSessions->end())
        {
            ipmiSessionsFlag = true;
        }

        // Note that control also reaches here via doPost and doDelete.
        auto session =
            persistent_data::SessionStore::getInstance().getSessionByUid(
                params[0]);
        if (session == nullptr && ipmiSessionsFlag != true)
        {
            messages::resourceNotFound(sessionAsyncResp->res, "Session",
                                       params[0]);
            sessionAsyncResp->res.end();
            return;
        }

        // Delete IPMI session from Redfish
        if (sessionDelete.value() == true && ipmiSessionsFlag)
        {
            messages::actionNotSupported(sessionAsyncResp->res,
                                         "deleting IPMI session from Redfish");
            BMCWEB_LOG_DEBUG
                << "Deleting IPMI session from Redfish is not allowed.";
            return;
        }
        if (ipmiSessionsFlag)
        {
            sessionAsyncResp->res.jsonValue["Id"] = params[0];
            sessionAsyncResp->res.jsonValue["UserName"] = "root";
            sessionAsyncResp->res.jsonValue["@odata.id"] =
                "/redfish/v1/SessionService/Sessions/" + params[0];
            sessionAsyncResp->res
                .jsonValue["Oem"]["OpenBMC"]["ClientOriginIP"] = "NA";
        }
        else
        {
            sessionAsyncResp->res.jsonValue["Id"] = session->uniqueId;
            sessionAsyncResp->res.jsonValue["UserName"] = session->username;
            sessionAsyncResp->res.jsonValue["@odata.id"] =
                "/redfish/v1/SessionService/Sessions/" + session->uniqueId;
            sessionAsyncResp->res
                .jsonValue["Oem"]["OpenBMC"]["ClientOriginIP"] =
                session->clientIp;
        }
        sessionAsyncResp->res.jsonValue["@odata.type"] =
            "#Session.v1_3_0.Session";
        sessionAsyncResp->res.jsonValue["Name"] = "User Session";
        sessionAsyncResp->res.jsonValue["Description"] = "Manager User Session";
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
        sessionAsyncResp->res.jsonValue["Oem"]["OpenBMC"]["@odata.type"] =
            "#OemSession.v1_0_0.Session";
        sessionAsyncResp->res.jsonValue["Oem"]["OpenBMC"]["ClientID"] =
            session->clientId;
#endif
        sessionAsyncResp->res.end();
        // Delete session from Redfish
        if (sessionDelete.value() == true)
        {
            if (ipmiSessionsFlag != true)
            {
                persistent_data::SessionStore::getInstance().removeSession(
                    session);
                return;
            }
        }
    }
    else
    {
        // This will support to get available sessions created from Redfish and
        // IPMI
        sessionAsyncResp->res.jsonValue["Members"] = nlohmann::json::array();
        for (const std::string& sid : *ipmiSessions)
        {
            if (sid.length() != 0)
            {
                sessionAsyncResp->res.jsonValue["Members"].push_back(
                    {{"@odata.id",
                      "/redfish/v1/SessionService/Sessions/" + sid}});
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
                    {{"@odata.id",
                      "/redfish/v1/SessionService/Sessions/" + *uid}});
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
}

inline void getSessionHandle(const std::shared_ptr<AsyncResp>& sessionAsyncResp,
                             const std::vector<std::string>& params,
                             std::optional<bool> sessionDelete)
{
    BMCWEB_LOG_DEBUG << "Get available Ipmi Sessions info.";

    std::array<std::string, 1> interfaces = {
        "xyz.openbmc_project.Ipmi.SessionInfo"};
    std::vector<std::string> ipmiSessions;

    auto sessionSharedPtr =
        std::make_shared<std::vector<std::string>>(ipmiSessions);

    crow::connections::systemBus->async_method_call(
        [sessionAsyncResp, sessionSharedPtr, params, sessionDelete](
            const boost::system::error_code ec, const GetSubTreeType& subtree) {
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
                getSessionInfo(sessionAsyncResp, sessionSharedPtr, params,
                               sessionDelete);
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
                        [sessionAsyncResp, sessionSharedPtr, params,
                         sessionDelete, objectServiceName, objPath,
                         subtreeEndPtr,
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
                                               sessionSharedPtr, params,
                                               sessionDelete);
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        getSessionHandle(asyncResp, params, false);
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

        // Perform a proper ConfigureSelf authority check.  If a
        // session is being used to DELETE some other user's session,
        // then the ConfigureSelf privilege does not apply.  In that
        // case, perform the authority check again without the user's
        // ConfigureSelf privilege.
        if (session != nullptr)
        {
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
        }

        // DELETE should return representation of object that will be removed
        std::optional<bool> sessionDelete = true;
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        getSessionHandle(asyncResp, params, sessionDelete);
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
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        getSessionHandle(asyncResp, params, false);
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
