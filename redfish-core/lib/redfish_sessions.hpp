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
        Node::json["@odata.type"] = "#Session.v1_0_2.Session";
        Node::json["@odata.context"] = "/redfish/v1/$metadata#Session.Session";
        Node::json["Name"] = "User Session";
        Node::json["Description"] = "Manager User Session";

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
        auto session =
            crow::persistent_data::SessionStore::getInstance().getSessionByUid(
                params[0]);

        if (session == nullptr)
        {
            messages::addMessageToErrorJson(
                res.jsonValue,
                messages::resourceNotFound("Session", params[0]));

            res.result(boost::beast::http::status::not_found);
            res.end();
            return;
        }

        Node::json["Id"] = session->uniqueId;
        Node::json["UserName"] = session->username;
        Node::json["@odata.id"] =
            "/redfish/v1/SessionService/Sessions/" + session->uniqueId;

        res.jsonValue = Node::json;
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

            res.result(boost::beast::http::status::bad_request);
            messages::addMessageToErrorJson(res.jsonValue,
                                            messages::generalError());

            res.end();
            return;
        }

        auto session =
            crow::persistent_data::SessionStore::getInstance().getSessionByUid(
                params[0]);

        if (session == nullptr)
        {
            messages::addMessageToErrorJson(
                res.jsonValue,
                messages::resourceNotFound("Session", params[0]));

            res.result(boost::beast::http::status::not_found);
            res.end();
            return;
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
        Node::json["@odata.type"] = "#SessionCollection.SessionCollection";
        Node::json["@odata.id"] = "/redfish/v1/SessionService/Sessions/";
        Node::json["@odata.context"] =
            "/redfish/v1/$metadata#SessionCollection.SessionCollection";
        Node::json["Name"] = "Session Collection";
        Node::json["Description"] = "Session Collection";
        Node::json["Members@odata.count"] = 0;
        Node::json["Members"] = nlohmann::json::array();

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

        Node::json["Members@odata.count"] = sessionIds.size();
        Node::json["Members"] = nlohmann::json::array();
        for (const std::string* uid : sessionIds)
        {
            Node::json["Members"].push_back(
                {{"@odata.id", "/redfish/v1/SessionService/Sessions/" + *uid}});
        }

        res.jsonValue = Node::json;
        res.end();
    }

    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>& params) override
    {
        boost::beast::http::status status;
        std::string username;
        bool userAuthSuccessful =
            authenticateUser(req, status, username, res.jsonValue);
        res.result(status);

        if (!userAuthSuccessful)
        {
            res.end();
            return;
        }

        // User is authenticated - create session for him
        auto session = crow::persistent_data::SessionStore::getInstance()
                           .generateUserSession(username);
        res.addHeader("X-Auth-Token", session->sessionToken);

        res.addHeader("Location", "/redfish/v1/SessionService/Sessions/" +
                                      session->uniqueId);

        // Return data for created session
        memberSession.doGet(res, req, {session->uniqueId});

        // No need for res.end(), as it is called by doGet()
    }

    /**
     * @brief Verifies data provided in request and tries to authenticate user
     *
     * @param[in]  req            Crow request containing authentication data
     * @param[out] httpRespCode   HTTP Code that should be returned in response
     * @param[out] user           Retrieved username - not filled on failure
     * @param[out] errJson        JSON to which error messages will be written
     *
     * @return true if authentication was successful, false otherwise
     */
    bool authenticateUser(const crow::Request& req,
                          boost::beast::http::status& httpRespCode,
                          std::string& user, nlohmann::json& errJson)
    {
        // We need only UserName and Password - nothing more, nothing less
        static constexpr const unsigned int numberOfRequiredFieldsInReq = 2;

        // call with exceptions disabled
        auto loginCredentials = nlohmann::json::parse(req.body, nullptr, false);
        if (loginCredentials.is_discarded())
        {
            httpRespCode = boost::beast::http::status::bad_request;

            messages::addMessageToErrorJson(errJson, messages::malformedJSON());

            return false;
        }

        // Check that there are only as many fields as there should be
        if (loginCredentials.size() != numberOfRequiredFieldsInReq)
        {
            httpRespCode = boost::beast::http::status::bad_request;

            messages::addMessageToErrorJson(errJson, messages::malformedJSON());

            return false;
        }

        // Find fields that we need - UserName and Password
        auto userIt = loginCredentials.find("UserName");
        auto passIt = loginCredentials.find("Password");
        if (userIt == loginCredentials.end() ||
            passIt == loginCredentials.end())
        {
            httpRespCode = boost::beast::http::status::bad_request;

            if (userIt == loginCredentials.end())
            {
                messages::addMessageToErrorJson(
                    errJson, messages::propertyMissing("UserName"));
            }

            if (passIt == loginCredentials.end())
            {
                messages::addMessageToErrorJson(
                    errJson, messages::propertyMissing("Password"));
            }

            return false;
        }

        // Check that given data is of valid type (string)
        if (!userIt->is_string() || !passIt->is_string())
        {
            httpRespCode = boost::beast::http::status::bad_request;

            if (!userIt->is_string())
            {
                messages::addMessageToErrorJson(
                    errJson, messages::propertyValueTypeError(userIt->dump(),
                                                              "UserName"));
            }

            if (!passIt->is_string())
            {
                messages::addMessageToErrorJson(
                    errJson, messages::propertyValueTypeError(userIt->dump(),
                                                              "Password"));
            }

            return false;
        }

        // Extract username and password
        std::string username = userIt->get<const std::string>();
        std::string password = passIt->get<const std::string>();

        // Verify that required fields are not empty
        if (username.empty() || password.empty())
        {
            httpRespCode = boost::beast::http::status::bad_request;

            if (username.empty())
            {
                messages::addMessageToErrorJson(
                    errJson, messages::propertyMissing("UserName"));
            }

            if (password.empty())
            {
                messages::addMessageToErrorJson(
                    errJson, messages::propertyMissing("Password"));
            }

            return false;
        }

        // Finally - try to authenticate user
        if (!pamAuthenticateUser(username, password))
        {
            httpRespCode = boost::beast::http::status::unauthorized;

            messages::addMessageToErrorJson(
                errJson,
                messages::resourceAtUriUnauthorized(
                    std::string(req.url), "Invalid username or password"));

            return false;
        }

        // User authenticated successfully
        httpRespCode = boost::beast::http::status::ok;
        user = username;

        return true;
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
        Node::json["@odata.type"] = "#SessionService.v1_0_2.SessionService";
        Node::json["@odata.id"] = "/redfish/v1/SessionService/";
        Node::json["@odata.context"] =
            "/redfish/v1/$metadata#SessionService.SessionService";
        Node::json["Name"] = "Session Service";
        Node::json["Id"] = "SessionService";
        Node::json["Description"] = "Session Service";
        Node::json["SessionTimeout"] =
            crow::persistent_data::SessionStore::getInstance()
                .getTimeoutInSeconds();
        Node::json["ServiceEnabled"] = true;

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
        res.jsonValue = Node::json;
        res.end();
    }
};

} // namespace redfish
