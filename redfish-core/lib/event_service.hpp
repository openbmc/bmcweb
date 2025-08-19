#pragma once
#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "event_service_manager.hpp"
#include "event_service_store.hpp"
#include "generated/enums/event_destination.hpp"
#include "http/utility.hpp"
#include "http_request.hpp"
#include "io_context_singleton.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries.hpp"
#include "registries/privilege_registry.hpp"
#include "snmp_trap_event_clients.hpp"
#include "subscription.hpp"
#include "utils/json_utils.hpp"

#include <asm-generic/errno.h>

#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/result.hpp>
#include <boost/url/format.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/url.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

namespace redfish
{

void requestRoutesEventService(App& app);
void requestRoutesSubmitTestEvent(App& app);
void requestRoutesEventDestinationCollection(App& app);
void requestRoutesEventDestination(App& app);

} // namespace redfish
