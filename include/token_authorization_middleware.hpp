#pragma once

#include <crow/http_request.h>
#include <crow/http_response.h>

namespace crow {
struct TokenAuthorizationMiddleware {
  struct context {
    std::string auth_token;
  };

  std::string auth_token2;

  void before_handle(crow::request& req, response& res, context& ctx);

  void after_handle(request& req, response& res, context& ctx);
};
}