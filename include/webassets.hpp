#pragma once

#include <string>

#include <crow/app.h>
#include <crow/http_request.h>
#include <crow/http_response.h>

#include <crow/routing.h>
#include <crow/bmc_app_type.hpp>


namespace crow {
namespace webassets {
void request_routes(BmcAppType& app);
}
}