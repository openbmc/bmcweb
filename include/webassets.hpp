#pragma once

#include <string>

#include <crow/app.h>
#include <crow/http_request.h>
#include <crow/http_response.h>

// TODO this is wrong.  file handler shouldn't care about middlewares
#include <crow/routing.h>
#include "token_authorization_middleware.hpp"

namespace crow {
namespace webassets {
void request_routes(crow::App<crow::TokenAuthorizationMiddleware>& app);
}
}