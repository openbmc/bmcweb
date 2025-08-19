#pragma once
#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utility.hpp"
#include "utils/journal_utils.hpp"
#include "utils/query_param.hpp"
#include "utils/time_utils.hpp"

#include <systemd/sd-journal.h>

#include <boost/asio/post.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace redfish
{

void requestRoutesBMCJournalLogService(App& app);

} // namespace redfish
