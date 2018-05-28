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
#include "crow.h"

namespace redfish {

/**
 * AsyncResp
 * Gathers data needed for response processing after async calls are done
 */
class AsyncResp {
 public:
  AsyncResp(crow::response& response) : res(response) {}

  ~AsyncResp() { res.end(); }

  crow::response& res;
};

/**
 * @brief  Abstract class used for implementing Redfish nodes.
 *
 */
class Node {
 public:
  template <typename... Params>
  Node(CrowApp& app, std::string&& entityUrl, Params... params) {
    app.route_dynamic(entityUrl.c_str())
        .methods("GET"_method, "PATCH"_method, "POST"_method,
                 "DELETE"_method)([&](const crow::request& req,
                                      crow::response& res, Params... params) {
          std::vector<std::string> paramVec = {params...};
          dispatchRequest(app, req, res, paramVec);
        });
  }

  virtual ~Node() = default;

  const std::string* getUrl() const {
    auto odataId = json.find("@odata.id");
    if (odataId == json.end()) {
      return nullptr;
    }

    return odataId->get_ptr<const std::string*>();
  }

  /**
   * @brief Inserts subroute fields into for the node's json in the form:
   *        "subroute_name" : { "odata.id": "node_url/subroute_name/" }
   *        Excludes metadata urls starting with "$" and child urls having
   *        more than one level.
   *
   * @return  None
   */
  void getSubRoutes(const std::vector<std::unique_ptr<Node>>& allNodes) {
    const std::string* url = getUrl();
    if (url == nullptr) {
      CROW_LOG_CRITICAL << "Unable to get url for route";
      return;
    }

    for (const auto& node : allNodes) {
      const std::string* route = node->getUrl();
      if (route == nullptr) {
        CROW_LOG_CRITICAL << "Unable to get url for route";
        continue;
      }
      if (boost::starts_with(*route, *url)) {
        std::string subRoute = route->substr(url->size());
        if (subRoute.empty()) {
          continue;
        }

        if (boost::starts_with(subRoute, "/")) {
          subRoute.erase(0, 1);
        }

        if (boost::ends_with(subRoute, "/")) {
          subRoute.pop_back();
        }

        if (!boost::starts_with(subRoute, "$") &&
            subRoute.find('/') == std::string::npos) {
          json[subRoute] = nlohmann::json{{"@odata.id", *route}};
        }
      }
    }
  }

  OperationMap entityPrivileges;

 protected:
  // Node is designed to be an abstract class, so doGet is pure virtual
  virtual void doGet(crow::response& res, const crow::request& req,
                     const std::vector<std::string>& params) = 0;

  virtual void doPatch(crow::response& res, const crow::request& req,
                       const std::vector<std::string>& params) {
    res.code = static_cast<int>(HttpRespCode::METHOD_NOT_ALLOWED);
    res.end();
  }

  virtual void doPost(crow::response& res, const crow::request& req,
                      const std::vector<std::string>& params) {
    res.code = static_cast<int>(HttpRespCode::METHOD_NOT_ALLOWED);
    res.end();
  }

  virtual void doDelete(crow::response& res, const crow::request& req,
                        const std::vector<std::string>& params) {
    res.code = static_cast<int>(HttpRespCode::METHOD_NOT_ALLOWED);
    res.end();
  }

  nlohmann::json json;

 private:
  void dispatchRequest(CrowApp& app, const crow::request& req,
                       crow::response& res,
                       const std::vector<std::string>& params) {
    auto ctx =
        app.template get_context<crow::TokenAuthorization::Middleware>(req);

    if (!isMethodAllowedForUser(req.method, entityPrivileges,
                                ctx.session->username)) {
      res.code = static_cast<int>(HttpRespCode::METHOD_NOT_ALLOWED);
      res.end();
      return;
    }

    switch (req.method) {
      case "GET"_method:
        doGet(res, req, params);
        break;

      case "PATCH"_method:
        doPatch(res, req, params);
        break;

      case "POST"_method:
        doPost(res, req, params);
        break;

      case "DELETE"_method:
        doDelete(res, req, params);
        break;

      default:
        res.code = static_cast<int>(HttpRespCode::NOT_FOUND);
        res.end();
    }
    return;
  }
};

}  // namespace redfish
