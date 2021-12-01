#pragma once

#include <app.hpp>

#define REDFISH_ROUTE(app, url)                                                 \
    app.template route<crow::black_magic::getParameterTag(url)>(url)