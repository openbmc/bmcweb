#pragma once
#include "bmcweb_config.h"
#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/pcie_device.hpp"
#include "generated/enums/pcie_slots.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/pcie_util.hpp"
#include <asm-generic/errno.h>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/unpack_properties.hpp>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>

namespace redfish
{

void requestRoutesSystemPCIeDeviceCollection(App& app);
void requestRoutesSystemPCIeDevice(App& app);
void requestRoutesSystemPCIeFunctionCollection(App& app);
void requestRoutesSystemPCIeFunction(App& app);

} // namespace redfish