#pragma once

#include <crow/app.h>
#include <token_authorization_middleware.hpp>

using BmcAppType = crow::App<crow::TokenAuthorizationMiddleware>;