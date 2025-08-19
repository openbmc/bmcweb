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
#include <boost/asio/error.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/linux_error.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <ratio>
#include <source_location>
#include <string>

namespace redfish
{

void setBytesProperty(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const nlohmann::json::json_pointer& jPtr,
    const boost::system::error_code& ec, double bytes);
void setPercentProperty(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const nlohmann::json::json_pointer& jPtr,
    const boost::system::error_code& ec, double userCPU);
void requestRoutesManagerDiagnosticData(App& app);

} // namespace redfish