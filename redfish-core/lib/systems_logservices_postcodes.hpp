#pragma once
#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/log_service.hpp"
#include "http_request.hpp"
#include "http_utility.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries.hpp"
#include "registries/privilege_registry.hpp"
#include "str_utility.hpp"
#include "utility.hpp"
#include "utils/hex_utils.hpp"
#include "utils/query_param.hpp"
#include "utils/time_utils.hpp"

#include <asm-generic/errno.h>

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/url/format.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <iomanip>
#include <ios>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <utility>
#include <vector>

namespace redfish
{

bool parsePostCode(std::string_view postCodeID, uint64_t& currentValue,
                   uint16_t& index);
void requestRoutesSystemsLogServicesPostCode(App& app);

} // namespace redfish
