#pragma once
#include "bmcweb_config.h"
#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/log_entry.hpp"
#include "generated/enums/log_service.hpp"
#include "http_body.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "http_utility.hpp"
#include "human_sort.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries.hpp"
#include "registries/privilege_registry.hpp"
#include "str_utility.hpp"
#include "task.hpp"
#include "task_messages.hpp"
#include "utils/dbus_event_log_entry.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"
#include "utils/query_param.hpp"
#include "utils/time_utils.hpp"
#include <asm-generic/errno.h>
#include <systemd/sd-bus.h>
#include <tinyxml2.h>
#include <unistd.h>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/linux_error.hpp>
#include <boost/url/format.hpp>
#include <boost/url/url.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

namespace redfish
{

void requestRoutesSystemLogServiceCollection(App& app);
void requestRoutesEventLogService(App& app);
void requestRoutesJournalEventLogClear(App& app);
void requestRoutesJournalEventLogEntryCollection(App& app);
void requestRoutesJournalEventLogEntry(App& app);
void requestRoutesDBusEventLogEntryCollection(App& app);
void requestRoutesDBusEventLogEntry(App& app);
void requestRoutesBMCLogServiceCollection(App& app);
void getDumpServiceInfo(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& dumpType);
void requestRoutesBMCDumpService(App& app);
void requestRoutesBMCDumpEntryCollection(App& app);
void requestRoutesBMCDumpEntry(App& app);
void requestRoutesBMCDumpEntryDownload(App& app);
void requestRoutesBMCDumpCreate(App& app);
void requestRoutesBMCDumpClear(App& app);
void requestRoutesDBusEventLogEntryDownload(App& app);
void requestRoutesFaultLogDumpService(App& app);
void requestRoutesFaultLogDumpEntryCollection(App& app);
void requestRoutesFaultLogDumpEntry(App& app);
void requestRoutesFaultLogDumpClear(App& app);
void requestRoutesSystemDumpService(App& app);
void requestRoutesSystemDumpEntryCollection(App& app);
void requestRoutesSystemDumpEntry(App& app);
void requestRoutesSystemDumpCreate(App& app);
void requestRoutesSystemDumpClear(App& app);
void requestRoutesCrashdumpService(App& app);
void requestRoutesCrashdumpClear(App& app);
void requestRoutesCrashdumpEntryCollection(App& app);
void requestRoutesCrashdumpEntry(App& app);
void requestRoutesCrashdumpFile(App& app);
void requestRoutesCrashdumpCollect(App& app);
void requestRoutesDBusLogServiceActionsClear(App& app);

} // namespace redfish