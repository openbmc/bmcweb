#pragma once

#include <string>

#include <crow/http_request.h>
#include <crow/http_response.h>
#include <crow/app.h>

//TODO this is wrong.  file handler shouldn't care about middlewares
#include "token_authorization_middleware.hpp"
#include <crow/routing.h>

namespace crow
{
namespace webassets
{
    void request_routes(crow::App<crow::TokenAuthorizationMiddleware>& app);
}
}