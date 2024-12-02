/****************************************************************
 *                 READ THIS WARNING FIRST
 * This is an auto-generated header which contains definitions
 * for Redfish DMTF defined messages.
 * DO NOT modify this registry outside of running the
 * parse_registries.py script.  The definitions contained within
 * this file are owned by DMTF.  Any modifications to these files
 * should be first pushed to the relevant registry in the DMTF
 * github organization.
 ***************************************************************/
#include "heartbeat_messages.hpp"

#include "http_response.hpp"
#include "logging.hpp"
#include "registries.hpp"
#include "registries/heartbeat_message_registry.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/url/url_view_base.hpp>
#include <nlohmann/json.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <source_location>
#include <span>
#include <string>
#include <string_view>

// Clang can't seem to decide whether this header needs to be included or not,
// and is inconsistent.  Include it for now
// NOLINTNEXTLINE(misc-include-cleaner)
#include <utility>

namespace redfish
{

namespace messages
{

static nlohmann::json getLog(redfish::registries::heartbeat::Index name,
                             std::span<const std::string_view> args)
{
    size_t index = static_cast<size_t>(name);
    if (index >= redfish::registries::heartbeat::registry.size())
    {
        return {};
    }
    return getLogFromRegistry(redfish::registries::heartbeat::header,
                              redfish::registries::heartbeat::registry, index,
                              args);
}

/**
 * @internal
 * @brief Formats RedfishServiceFunctional message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json redfishServiceFunctional()
{
    return getLog(
        redfish::registries::heartbeat::Index::redfishServiceFunctional, {});
}

} // namespace messages
} // namespace redfish
