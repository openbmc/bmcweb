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
#include <tuple>
#include "node.hpp"
#include "session_storage_singleton.hpp"

namespace redfish {

class SessionCollection;

class Sessions : public Node {
 public:
  template <typename CrowApp, typename PrivilegeProvider>
  Sessions(CrowApp& app, PrivilegeProvider& provider)
      : Node(app, provider, "#Session.v1_0_2.Session",
             "/redfish/v1/SessionService/Sessions/<str>", std::string()) {
    nodeJson["@odata.type"] = Node::odataType;
    nodeJson["@odata.context"] = "/redfish/v1/$metadata#Session.Session";
    nodeJson["Name"] = "User Session";
    nodeJson["Description"] = "Manager User Session";
  }

 private:
  void doGet(crow::response& res, const crow::request& req,
             const std::vector<std::string>& params) override {
    auto session =
        crow::PersistentData::session_store->get_session_by_uid(params[0]);

    if (session == nullptr) {
      res.code = static_cast<int>(HttpRespCode::NOT_FOUND);
      res.end();
      return;
    }

    nodeJson["Id"] = session->unique_id;
    nodeJson["UserName"] = session->username;
    nodeJson["@odata.id"] =
        "/redfish/v1/SessionService/Sessions/" + session->unique_id;

    res.json_value = nodeJson;
    res.end();
  }

  void doDelete(crow::response& res, const crow::request& req,
                const std::vector<std::string>& params) override {
    // Need only 1 param which should be id of session to be deleted
    if (params.size() != 1) {
      res.code = static_cast<int>(HttpRespCode::BAD_REQUEST);
      res.end();
      return;
    }

    auto session =
        crow::PersistentData::session_store->get_session_by_uid(params[0]);

    if (session == nullptr) {
      res.code = static_cast<int>(HttpRespCode::NOT_FOUND);
      res.end();
      return;
    }

    crow::PersistentData::session_store->remove_session(session);
    res.code = static_cast<int>(HttpRespCode::OK);
    res.end();
  }

  /**
   * This allows SessionCollection to reuse this class' doGet method, to
   * maintain consistency of returned data, as Collection's doPost should return
   * data for created member which should match member's doGet result in 100%
   */
  friend SessionCollection;

  nlohmann::json nodeJson;
};

class SessionCollection : public Node {
 public:
  template <typename CrowApp, typename PrivilegeProvider>
  SessionCollection(CrowApp& app, PrivilegeProvider& provider)
      : Node(app, provider, "#SessionCollection.SessionCollection",
             "/redfish/v1/SessionService/Sessions/"),
        memberSession(app, provider) {
    nodeJson["@odata.type"] = Node::odataType;
    nodeJson["@odata.id"] = Node::odataId;
    nodeJson["@odata.context"] =
        "/redfish/v1/$metadata#SessionCollection.SessionCollection";
    nodeJson["Name"] = "Session Collection";
    nodeJson["Description"] = "Session Collection";
    nodeJson["Members@odata.count"] = 0;
    nodeJson["Members"] = nlohmann::json::array();
  }

 private:
  void doGet(crow::response& res, const crow::request& req,
             const std::vector<std::string>& params) override {
    std::vector<const std::string*> session_ids =
        crow::PersistentData::session_store->get_unique_ids(
            false, crow::PersistentData::PersistenceType::TIMEOUT);

    nodeJson["Members@odata.count"] = session_ids.size();
    nodeJson["Members"] = nlohmann::json::array();
    for (const auto& uid : session_ids) {
      nodeJson["Members"].push_back(
          {{"@odata.id", "/redfish/v1/SessionService/Sessions/" + *uid}});
    }

    res.json_value = nodeJson;
    res.end();
  }

  void doPost(crow::response& res, const crow::request& req,
              const std::vector<std::string>& params) override {
    std::string username;
    bool userAuthSuccessful = authenticateUser(req, &res.code, &username);

    if (!userAuthSuccessful) {
      res.end();
      return;
    }

    // User is authenticated - create session for him
    auto session =
        crow::PersistentData::session_store->generate_user_session(username);
    res.add_header("X-Auth-Token", session.session_token);

    // Return data for created session
    memberSession.doGet(res, req, {session.unique_id});

    // No need for res.end(), as it is called by doGet()
  }

  /**
   * @brief Verifies data provided in request and tries to authenticate user
   *
   * @param[in]  req            Crow request containing authentication data
   * @param[out] httpRespCode   HTTP Code that should be returned in response
   * @param[out] user           Retrieved username - not filled on failure
   *
   * @return true if authentication was successful, false otherwise
   */
  bool authenticateUser(const crow::request& req, int* httpRespCode,
                        std::string* user) {
    // We need only UserName and Password - nothing more, nothing less
    static constexpr const unsigned int numberOfRequiredFieldsInReq = 2;

    // call with exceptions disabled
    auto login_credentials = nlohmann::json::parse(req.body, nullptr, false);
    if (login_credentials.is_discarded()) {
      *httpRespCode = static_cast<int>(HttpRespCode::BAD_REQUEST);

      return false;
    }

    // Check that there are only as many fields as there should be
    if (login_credentials.size() != numberOfRequiredFieldsInReq) {
      *httpRespCode = static_cast<int>(HttpRespCode::BAD_REQUEST);

      return false;
    }

    // Find fields that we need - UserName and Password
    auto user_it = login_credentials.find("UserName");
    auto pass_it = login_credentials.find("Password");
    if (user_it == login_credentials.end() ||
        pass_it == login_credentials.end()) {
      *httpRespCode = static_cast<int>(HttpRespCode::BAD_REQUEST);

      return false;
    }

    // Check that given data is of valid type (string)
    if (!user_it->is_string() || !pass_it->is_string()) {
      *httpRespCode = static_cast<int>(HttpRespCode::BAD_REQUEST);

      return false;
    }

    // Extract username and password
    std::string username = user_it->get<const std::string>();
    std::string password = pass_it->get<const std::string>();

    // Verify that required fields are not empty
    if (username.empty() || password.empty()) {
      *httpRespCode = static_cast<int>(HttpRespCode::BAD_REQUEST);

      return false;
    }

    // Finally - try to authenticate user
    if (!pam_authenticate_user(username, password)) {
      *httpRespCode = static_cast<int>(HttpRespCode::UNAUTHORIZED);

      return false;
    }

    // User authenticated successfully
    *httpRespCode = static_cast<int>(HttpRespCode::OK);
    *user = username;

    return true;
  }

  /**
   * Member session to ensure consistency between collection's doPost and
   * member's doGet, as they should return 100% matching data
   */
  Sessions memberSession;
  nlohmann::json nodeJson;
};

}  // namespace redfish
