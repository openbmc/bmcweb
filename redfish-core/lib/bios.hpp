#pragma once
#include "bmcweb_config.h"
#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/sw_utils.hpp"
#include <boost/beast/http/verb.hpp>
#include <format>
#include <functional>
#include <memory>
#include <string>

namespace redfish
{

void requestRoutesBiosService(App& app);
void requestRoutesBiosReset(App& app);

} // namespace redfish