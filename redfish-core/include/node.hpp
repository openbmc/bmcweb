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
#include "crow.h"

namespace redfish {

/**
 * @brief  Abstract class used for implementing Redfish nodes.
 *
 */
class Node {
 public:
  template <typename CrowApp, typename... Params>
  Node(CrowApp& app, PrivilegeProvider& provider, std::string odataType,
       std::string odataId, Params... params)
      : odataType(odataType), odataId(odataId) {
    // privileges for the node as defined in the privileges_registry.json
    entityPrivileges = provider.getPrivileges(odataId, odataType);

    app.route_dynamic(std::move(odataId))
        .methods("GET"_method, "PATCH"_method, "POST"_method,
                 "DELETE"_method)([&](const crow::request& req,
                                      crow::response& res, Params... params) {
          std::vector<std::string> paramVec = {params...};
          dispatchRequest(app, req, res, paramVec);
        });
  }

  template <typename CrowApp>
  void dispatchRequest(CrowApp& app, const crow::request& req,
                       crow::response& res,
                       const std::vector<std::string>& params) {
    // drop requests without required privileges
    auto ctx =
        app.template get_context<crow::TokenAuthorization::Middleware>(req);

    if (!entityPrivileges.isMethodAllowed(req.method, ctx.session->username)) {
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

 protected:
  const std::string odataType;
  const std::string odataId;

  // Node is designed to be an abstract class, so doGet is pure virutal
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

  EntityPrivileges entityPrivileges;
};

template <typename CrowApp>
void getRedfishSubRoutes(CrowApp& app, const std::string& url,
                         nlohmann::json& j) {
  std::vector<const std::string*> routes = app.get_routes(url);

  for (auto route : routes) {
    auto redfishSubRoute =
        route->substr(url.size(), route->size() - url.size() - 1);

    // Exclude: - exact matches,
    //          - metadata urls starting with "$",
    //          - urls at the same level
    if (!redfishSubRoute.empty() && redfishSubRoute[0] != '$' &&
        redfishSubRoute.find('/') == std::string::npos) {
      j[redfishSubRoute] = nlohmann::json{{"@odata.id", *route}};
    }
  }
}

}  // namespace redfish

