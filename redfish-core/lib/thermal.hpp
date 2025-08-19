#pragma once
#include "app.hpp"
#include "async_resp.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "sensors_async_response.hpp"
#include "utils/json_utils.hpp"
#include "utils/sensor_utils.hpp"

#include <boost/beast/http/verb.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace redfish
{

void requestRoutesThermal(App& app);

} // namespace redfish
