#pragma once

#include <crow/http_request.h>
#include <crow/http_response.h>

namespace crow {

struct User {

};

struct TokenAuthorizationMiddleware {
  // TODO(ed) auth_token shouldn't really be passed to the context
  // it opens the possibility of exposure by and endpoint.
  // instead we should only pass some kind of "user" struct
  struct context {
    std::string auth_token;
  };

  TokenAuthorizationMiddleware();

  void before_handle(crow::request& req, response& res, context& ctx);

  void after_handle(request& req, response& res, context& ctx);

  private:
    std::string auth_token2;
};
}