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

#include "registries.hpp"
#include "registries/heartbeat_event_message_registry.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
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

static nlohmann::json getLog(redfish::registries::heartbeat_event::Index name,
                             std::span<const std::string_view> args)
{
    size_t index = static_cast<size_t>(name);
    if (index >= redfish::registries::heartbeat_event::registry.size())
    {
        return {};
    }
    return getLogFromRegistry(redfish::registries::heartbeat_event::header,
                              redfish::registries::heartbeat_event::registry,
                              index, args);
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
        redfish::registries::heartbeat_event::Index::redfishServiceFunctional,
        {});
}

} // namespace messages
} // namespace redfish
