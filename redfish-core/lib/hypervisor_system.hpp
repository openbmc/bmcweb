#pragma once
#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "ethernet.hpp"
#include "generated/enums/action_info.hpp"
#include "generated/enums/computer_system.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/ip_utils.hpp"
#include "utils/json_utils.hpp"

#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace redfish
{

void handleHypervisorSystemGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp);
void handleHypervisorResetActionGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp);
void handleHypervisorSystemResetPost(
    const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp);
void requestRoutesHypervisorSystems(App& app);

} // namespace redfish
