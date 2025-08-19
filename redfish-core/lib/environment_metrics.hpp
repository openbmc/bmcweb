#pragma once
#include "app.hpp"
#include "async_resp.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/chassis_utils.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace redfish
{

void requestRoutesEnvironmentMetrics(App& app);

} // namespace redfish
