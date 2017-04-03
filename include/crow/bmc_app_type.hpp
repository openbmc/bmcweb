#pragma once

#include <crow/app.h>
#include <token_authorization_middleware.hpp>
#include <security_headers_middleware.hpp>

using BmcAppType = crow::App<crow::TokenAuthorizationMiddleware, crow::SecurityHeadersMiddleware>;