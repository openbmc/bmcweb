#pragma once
#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "telemetry_readings.hpp"
#include "utils/collection.hpp"
#include "utils/telemetry_utils.hpp"
#include "utils/time_utils.hpp"
#include <asm-generic/errno.h>
#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>
#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/asio/property.hpp>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace redfish
{

void requestRoutesMetricReportCollection(App& app);
void requestRoutesMetricReport(App& app);

} // namespace redfish