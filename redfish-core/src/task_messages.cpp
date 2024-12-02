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
#include "task_messages.hpp"

#include "http_response.hpp"
#include "logging.hpp"
#include "registries.hpp"
#include "registries/task_event_message_registry.hpp"

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

static nlohmann::json getLog(redfish::registries::task_event::Index name,
                             std::span<const std::string_view> args)
{
    size_t index = static_cast<size_t>(name);
    if (index >= redfish::registries::task_event::registry.size())
    {
        return {};
    }
    return getLogFromRegistry(redfish::registries::task_event::header,
                              redfish::registries::task_event::registry, index,
                              args);
}

/**
 * @internal
 * @brief Formats TaskStarted message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json taskStarted(std::string_view arg1)
{
    return getLog(redfish::registries::task_event::Index::taskStarted,
                  std::to_array({arg1}));
}

/**
 * @internal
 * @brief Formats TaskCompletedOK message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json taskCompletedOK(std::string_view arg1)
{
    return getLog(redfish::registries::task_event::Index::taskCompletedOK,
                  std::to_array({arg1}));
}

/**
 * @internal
 * @brief Formats TaskCompletedWarning message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json taskCompletedWarning(std::string_view arg1)
{
    return getLog(redfish::registries::task_event::Index::taskCompletedWarning,
                  std::to_array({arg1}));
}

/**
 * @internal
 * @brief Formats TaskAborted message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json taskAborted(std::string_view arg1)
{
    return getLog(redfish::registries::task_event::Index::taskAborted,
                  std::to_array({arg1}));
}

/**
 * @internal
 * @brief Formats TaskCancelled message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json taskCancelled(std::string_view arg1)
{
    return getLog(redfish::registries::task_event::Index::taskCancelled,
                  std::to_array({arg1}));
}

/**
 * @internal
 * @brief Formats TaskRemoved message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json taskRemoved(std::string_view arg1)
{
    return getLog(redfish::registries::task_event::Index::taskRemoved,
                  std::to_array({arg1}));
}

/**
 * @internal
 * @brief Formats TaskPaused message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json taskPaused(std::string_view arg1)
{
    return getLog(redfish::registries::task_event::Index::taskPaused,
                  std::to_array({arg1}));
}

/**
 * @internal
 * @brief Formats TaskResumed message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json taskResumed(std::string_view arg1)
{
    return getLog(redfish::registries::task_event::Index::taskResumed,
                  std::to_array({arg1}));
}

/**
 * @internal
 * @brief Formats TaskProgressChanged message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json taskProgressChanged(std::string_view arg1, uint64_t arg2)
{
    std::string arg2Str = std::to_string(arg2);
    return getLog(redfish::registries::task_event::Index::taskProgressChanged,
                  std::to_array<std::string_view>({arg1, arg2Str}));
}

} // namespace messages
} // namespace redfish
