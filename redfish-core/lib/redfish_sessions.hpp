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
#include "persistent_data.hpp"

#include <app.hpp>
#include <registries/privilege_registry.hpp>

namespace redfish
{

class SessionCollection;

inline void fillSessionObject(crow::Response& res,
                              const persistent_data::UserSession& session)
{
    res.jsonValue["Id"] = session.uniqueId;
    res.jsonValue["UserName"] = session.username;
    res.jsonValue["@odata.id"] =
        "/redfish/v1/SessionService/Sessions/" + session.uniqueId;
    res.jsonValue["@odata.type"] = "#Session.v1_3_0.Session";
    res.jsonValue["Name"] = "User Session";
    res.jsonValue["Description"] = "Manager User Session";
    res.jsonValue["ClientOriginIPAddress"] = session.clientIp;
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
    res.jsonValue["Oem"]["OpenBMC"]["@odata.type"] =
        "#OemSession.v1_0_0.Session";
    res.jsonValue["Oem"]["OpenBMC"]["ClientID"] = session.clientId;
#endif
}

inline void requestRoutesSession(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/SessionService/Sessions/<str>/")
        .privileges(redfish::privileges::getSession)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request& /*req*/,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& sessionId) -> void {
                // Note that control also reaches here via doPost and doDelete.
                auto session = persistent_data::SessionStore::getInstance()
                                   .getSessionByUid(sessionId);

                if (session)
                {
                    fillSessionObject(asyncResp->res, *session);
                    return;
                }

                std::array<std::string, 1> interfaces = {
                    "xyz.openbmc_project.Ipmi.SessionInfo"};
                crow::connections::systemBus->async_method_call(
                    [asyncResp, sessionId](const boost::system::error_code ec,
                                           const GetSubTreeType& subtree) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG
                                << "Error in querying GetSubTree with "
                                   "Object Mapper. "
                                << ec;
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        if (subtree.size() == 0)
                        {
                            BMCWEB_LOG_DEBUG
                                << "Can't find  Session Info Attributes!";
                            messages::resourceNotFound(asyncResp->res,
                                                       "Session", sessionId);
                            return;
                        }
                        bool ipmiSessionFound = false;
                        std::string ipmiSessionService;
                        std::string ipmiSessionInfPath;
                        for (const auto& [ipmiSessionPath, object] : subtree)
                        {
                            if (ipmiSessionPath.empty() || object.empty())
                            {
                                BMCWEB_LOG_DEBUG
                                    << "Session Info Attributes mapper error!";
                                continue;
                            }
                            if (!boost::ends_with(ipmiSessionPath, sessionId))
                            {
                                continue;
                            }
                            ipmiSessionFound = true;
                            ipmiSessionService = object[0].first;
                            ipmiSessionInfPath = ipmiSessionPath;
                            break;
                        }
                        if (!ipmiSessionFound)
                        {
                            messages::resourceNotFound(asyncResp->res,
                                                       "Session", sessionId);
                            return;
                        }
                        if (ipmiSessionService.empty())
                        {
                            BMCWEB_LOG_DEBUG
                                << "Session Info Attributes mapper "
                                   "error!";
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        crow::connections::systemBus->async_method_call(
                            [asyncResp, sessionId](
                                const boost::system::error_code ec,
                                const std::vector<std::pair<
                                    std::string,
                                    std::variant<std::monostate, std::string,
                                                 uint32_t>>>& properties) {
                                if (ec)
                                {
                                    BMCWEB_LOG_DEBUG
                                        << "Error in querying Session "
                                           "Info State property "
                                        << ec;
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                std::string userName = "";
                                uint32_t remoteIpAddr;
                                try
                                {
                                    sdbusplus::unpackProperties(
                                        properties, "Username", userName,
                                        "RemoteIPAddr", remoteIpAddr);
                                    asyncResp->res.jsonValue["Id"] = sessionId;
                                    asyncResp->res.jsonValue["UserName"] =
                                        userName;
                                    asyncResp->res.jsonValue["@odata.id"] =
                                        "/redfish/v1/SessionService/"
                                        "Sessions/" +
                                        sessionId;
                                    asyncResp->res.jsonValue["@odata.type"] =
                                        "#Session.v1_3_0.Session";
                                    asyncResp->res.jsonValue["Name"] =
                                        "User Session";
                                    asyncResp->res.jsonValue["Description"] =
                                        "Manager User Session";
                                    struct in_addr ipAddr;
                                    ipAddr.s_addr = remoteIpAddr;
                                    asyncResp->res
                                        .jsonValue["ClientOriginIPAddress"] =
                                        inet_ntoa(ipAddr);
                                }
                                catch (const sdbusplus::exception::
                                           UnpackPropertyError& error)
                                {
                                    BMCWEB_LOG_ERROR << error.what();
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                return;
                            },
                            ipmiSessionService, ipmiSessionInfPath,
                            "org.freedesktop.DBus.Properties", "GetAll",
                            "xyz.openbmc_project.Ipmi.SessionInfo");
                    },
                    "xyz.openbmc_project.ObjectMapper",
                    "/xyz/openbmc_project/object_mapper",
                    "xyz.openbmc_project.ObjectMapper", "GetSubTree", "/", 0,
                    interfaces);
            });

    BMCWEB_ROUTE(app, "/redfish/v1/SessionService/Sessions/<str>/")
        .privileges(redfish::privileges::deleteSession)
        .methods(boost::beast::http::verb::delete_)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& sessionId) -> void {
                auto session = persistent_data::SessionStore::getInstance()
                                   .getSessionByUid(sessionId);

                // Perform a proper ConfigureSelf authority check.  If a
                // session is being used to DELETE some other user's session,
                // then the ConfigureSelf privilege does not apply.  In that
                // case, perform the authority check again without the user's
                // ConfigureSelf privilege.
                if (session)
                {
                    if (session->username != req.session->username)
                    {
                        Privileges effectiveUserPrivileges =
                            redfish::getUserPrivileges(req.userRole);

                        if (!effectiveUserPrivileges.isSupersetOf(
                                {"ConfigureUsers"}))
                        {
                            messages::insufficientPrivilege(asyncResp->res);
                            return;
                        }
                    }
                    persistent_data::SessionStore::getInstance().removeSession(
                        session);
                    messages::success(asyncResp->res);
                    return;
                }

                std::array<std::string, 1> interfaces = {
                    "xyz.openbmc_project.Ipmi.SessionInfo"};
                crow::connections::systemBus->async_method_call(
                    [asyncResp,
                     sessionId](const boost::system::error_code ec,
                                const std::vector<std::string>& ifaceList) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG
                                << "Error in querying GetSubTreePaths "
                                   "with Object Mapper. "
                                << ec;
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        if (ifaceList.size() == 0)
                        {
                            BMCWEB_LOG_DEBUG
                                << "Can't find  Session Info Attributes!";
                            return;
                        }
                        bool ipmiSessionFound = false;
                        for (const std::string& ipmiSessionPath : ifaceList)
                        {
                            if (!boost::ends_with(ipmiSessionPath, sessionId))
                            {
                                continue;
                            }
                            ipmiSessionFound = true;
                            break;
                        }
                        if (ipmiSessionFound)
                        {
                            BMCWEB_LOG_DEBUG << "Deleting IPMI session from "
                                                "Redfish is not allowed.";
                            messages::actionNotSupported(
                                asyncResp->res,
                                "deleting IPMI session from Redfish");
                            return;
                        }
                        messages::resourceNotFound(asyncResp->res, "Session",
                                                   sessionId);
                        return;
                    },
                    "xyz.openbmc_project.ObjectMapper",
                    "/xyz/openbmc_project/object_mapper",
                    "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths", "/",
                    0, interfaces);
            });

    BMCWEB_ROUTE(app, "/redfish/v1/SessionService/Sessions/")
        .privileges(redfish::privileges::getSessionCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request& /*req*/,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) -> void {
                std::vector<const std::string*> sessionIds =
                    persistent_data::SessionStore::getInstance().getUniqueIds(
                        false, persistent_data::PersistenceType::TIMEOUT);

                asyncResp->res.jsonValue["Members@odata.count"] =
                    sessionIds.size();
                asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
                for (const std::string* uid : sessionIds)
                {
                    asyncResp->res.jsonValue["Members"].push_back(
                        {{"@odata.id",
                          "/redfish/v1/SessionService/Sessions/" + *uid}});
                }
                asyncResp->res.jsonValue["Members@odata.count"] =
                    sessionIds.size();
                asyncResp->res.jsonValue["@odata.type"] =
                    "#SessionCollection.SessionCollection";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/SessionService/Sessions/";
                asyncResp->res.jsonValue["Name"] = "Session Collection";
                asyncResp->res.jsonValue["Description"] = "Session Collection";

                std::array<std::string, 1> interfaces = {
                    "xyz.openbmc_project.Ipmi.SessionInfo"};
                crow::connections::systemBus->async_method_call(
                    [asyncResp](const boost::system::error_code ec,
                                const std::vector<std::string>& ifaceList) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG
                                << "Error in querying GetSubTreePaths "
                                   "with Object Mapper. "
                                << ec;
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        if (ifaceList.size() == 0)
                        {
                            BMCWEB_LOG_DEBUG
                                << "Can't find  Session Info Attributes!";
                            return;
                        }
                        for (const std::string& ipmiSessionPath : ifaceList)
                        {
                            std::filesystem::path filePath(ipmiSessionPath);
                            std::string ipmiSessionID =
                                filePath.has_filename() ? filePath.filename()
                                                        : "";
                            if (!ipmiSessionID.empty() && ipmiSessionID != "0")
                            {
                                asyncResp->res.jsonValue["Members"].push_back(
                                    {{"@odata.id",
                                      "/redfish/v1/SessionService/Sessions/" +
                                          ipmiSessionID}});
                            }
                        }
                        asyncResp->res.jsonValue["Members@odata.count"] =
                            asyncResp->res.jsonValue["Members"].size();
                    },
                    "xyz.openbmc_project.ObjectMapper",
                    "/xyz/openbmc_project/object_mapper",
                    "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths", "/",
                    0, interfaces);
            });

    BMCWEB_ROUTE(app, "/redfish/v1/SessionService/Sessions/")
        // Note, this technically doesn't match the privilege registry given the
        // way login mechanisms work.  The base privilege registry lists this
        // endpoint as requiring login privilege, but because this is the
        // endpoint responsible for giving the login privilege, and it is itself
        // its own route, it needs to not require Login
        .privileges({})
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) -> void {
                std::string username;
                std::string password;
                std::optional<nlohmann::json> oemObject;
                std::string clientId;
                if (!json_util::readJson(req, asyncResp->res, "UserName",
                                         username, "Password", password, "Oem",
                                         oemObject))
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
                    messages::resourceAtUriUnauthorized(
                        asyncResp->res, std::string(req.url),
                        "Invalid username or password");
                    return;
                }
#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
                if (oemObject)
                {
                    std::optional<nlohmann::json> bmcOem;
                    if (!json_util::readJson(*oemObject, asyncResp->res,
                                             "OpenBMC", bmcOem))
                    {
                        return;
                    }
                    if (!json_util::readJson(*bmcOem, asyncResp->res,
                                             "ClientID", clientId))
                    {
                        BMCWEB_LOG_ERROR << "Could not read ClientId";
                        return;
                    }
                }
#endif

                // User is authenticated - create session
                std::shared_ptr<persistent_data::UserSession> session =
                    persistent_data::SessionStore::getInstance()
                        .generateUserSession(
                            username, req.ipAddress.to_string(), clientId,
                            persistent_data::PersistenceType::TIMEOUT,
                            isConfigureSelfOnly);
                asyncResp->res.addHeader("X-Auth-Token", session->sessionToken);
                asyncResp->res.addHeader(
                    "Location",
                    "/redfish/v1/SessionService/Sessions/" + session->uniqueId);
                asyncResp->res.result(boost::beast::http::status::created);
                if (session->isConfigureSelfOnly)
                {
                    messages::passwordChangeRequired(
                        asyncResp->res, "/redfish/v1/AccountService/Accounts/" +
                                            session->username);
                }

                fillSessionObject(asyncResp->res, *session);
            });

    BMCWEB_ROUTE(app, "/redfish/v1/SessionService/")
        .privileges(redfish::privileges::getSessionService)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request& /* req */,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) -> void {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#SessionService.v1_0_2.SessionService";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/SessionService/";
                asyncResp->res.jsonValue["Name"] = "Session Service";
                asyncResp->res.jsonValue["Id"] = "SessionService";
                asyncResp->res.jsonValue["Description"] = "Session Service";
                asyncResp->res.jsonValue["SessionTimeout"] =
                    persistent_data::SessionStore::getInstance()
                        .getTimeoutInSeconds();
                asyncResp->res.jsonValue["ServiceEnabled"] = true;

                asyncResp->res.jsonValue["Sessions"] = {
                    {"@odata.id", "/redfish/v1/SessionService/Sessions"}};
            });

    BMCWEB_ROUTE(app, "/redfish/v1/SessionService/")
        .privileges(redfish::privileges::patchSessionService)
        .methods(boost::beast::http::verb::patch)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) -> void {
                std::optional<int64_t> sessionTimeout;
                if (!json_util::readJson(req, asyncResp->res, "SessionTimeout",
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
                        std::chrono::seconds sessionTimeoutInseconds(
                            *sessionTimeout);
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
            });
}

} // namespace redfish
