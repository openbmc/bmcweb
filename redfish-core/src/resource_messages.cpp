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
#include "resource_messages.hpp"

#include "registries.hpp"
#include "registries/resource_event_message_registry.hpp"

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

static nlohmann::json getLog(redfish::registries::resource_event::Index name,
                             std::span<const std::string_view> args)
{
    size_t index = static_cast<size_t>(name);
    if (index >= redfish::registries::resource_event::registry.size())
    {
        return {};
    }
    return getLogFromRegistry(redfish::registries::resource_event::header,
                              redfish::registries::resource_event::registry,
                              index, args);
}

/**
 * @internal
 * @brief Formats ResourceCreated message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceCreated()
{
    return getLog(redfish::registries::resource_event::Index::resourceCreated,
                  {});
}

/**
 * @internal
 * @brief Formats ResourceRemoved message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceRemoved()
{
    return getLog(redfish::registries::resource_event::Index::resourceRemoved,
                  {});
}

/**
 * @internal
 * @brief Formats ResourceErrorsDetected message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceErrorsDetected(std::string_view arg1,
                                      std::string_view arg2)
{
    return getLog(
        redfish::registries::resource_event::Index::resourceErrorsDetected,
        std::to_array({arg1, arg2}));
}

/**
 * @internal
 * @brief Formats ResourceErrorsCorrected message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceErrorsCorrected(std::string_view arg1,
                                       std::string_view arg2)
{
    return getLog(
        redfish::registries::resource_event::Index::resourceErrorsCorrected,
        std::to_array({arg1, arg2}));
}

/**
 * @internal
 * @brief Formats ResourceErrorThresholdExceeded message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json
    resourceErrorThresholdExceeded(std::string_view arg1, std::string_view arg2)
{
    return getLog(redfish::registries::resource_event::Index::
                      resourceErrorThresholdExceeded,
                  std::to_array({arg1, arg2}));
}

/**
 * @internal
 * @brief Formats ResourceErrorThresholdCleared message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json
    resourceErrorThresholdCleared(std::string_view arg1, std::string_view arg2)
{
    return getLog(redfish::registries::resource_event::Index::
                      resourceErrorThresholdCleared,
                  std::to_array({arg1, arg2}));
}

/**
 * @internal
 * @brief Formats ResourceWarningThresholdExceeded message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceWarningThresholdExceeded(std::string_view arg1,
                                                std::string_view arg2)
{
    return getLog(redfish::registries::resource_event::Index::
                      resourceWarningThresholdExceeded,
                  std::to_array({arg1, arg2}));
}

/**
 * @internal
 * @brief Formats ResourceWarningThresholdCleared message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceWarningThresholdCleared(std::string_view arg1,
                                               std::string_view arg2)
{
    return getLog(redfish::registries::resource_event::Index::
                      resourceWarningThresholdCleared,
                  std::to_array({arg1, arg2}));
}

/**
 * @internal
 * @brief Formats ResourceStatusChangedOK message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceStatusChangedOK(std::string_view arg1,
                                       std::string_view arg2)
{
    return getLog(
        redfish::registries::resource_event::Index::resourceStatusChangedOK,
        std::to_array({arg1, arg2}));
}

/**
 * @internal
 * @brief Formats ResourceStatusChangedWarning message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json
    resourceStatusChangedWarning(std::string_view arg1, std::string_view arg2)
{
    return getLog(redfish::registries::resource_event::Index::
                      resourceStatusChangedWarning,
                  std::to_array({arg1, arg2}));
}

/**
 * @internal
 * @brief Formats ResourceStatusChangedCritical message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json
    resourceStatusChangedCritical(std::string_view arg1, std::string_view arg2)
{
    return getLog(redfish::registries::resource_event::Index::
                      resourceStatusChangedCritical,
                  std::to_array({arg1, arg2}));
}

/**
 * @internal
 * @brief Formats ResourceStateChanged message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceStateChanged(std::string_view arg1,
                                    std::string_view arg2)
{
    return getLog(
        redfish::registries::resource_event::Index::resourceStateChanged,
        std::to_array({arg1, arg2}));
}

/**
 * @internal
 * @brief Formats ResourcePoweredOn message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourcePoweredOn(std::string_view arg1)
{
    return getLog(redfish::registries::resource_event::Index::resourcePoweredOn,
                  std::to_array({arg1}));
}

/**
 * @internal
 * @brief Formats ResourcePoweringOn message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourcePoweringOn(std::string_view arg1)
{
    return getLog(
        redfish::registries::resource_event::Index::resourcePoweringOn,
        std::to_array({arg1}));
}

/**
 * @internal
 * @brief Formats ResourcePoweredOff message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourcePoweredOff(std::string_view arg1)
{
    return getLog(
        redfish::registries::resource_event::Index::resourcePoweredOff,
        std::to_array({arg1}));
}

/**
 * @internal
 * @brief Formats ResourcePoweringOff message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourcePoweringOff(std::string_view arg1)
{
    return getLog(
        redfish::registries::resource_event::Index::resourcePoweringOff,
        std::to_array({arg1}));
}

/**
 * @internal
 * @brief Formats ResourcePaused message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourcePaused(std::string_view arg1)
{
    return getLog(redfish::registries::resource_event::Index::resourcePaused,
                  std::to_array({arg1}));
}

/**
 * @internal
 * @brief Formats URIForResourceChanged message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json uRIForResourceChanged()
{
    return getLog(
        redfish::registries::resource_event::Index::uRIForResourceChanged, {});
}

/**
 * @internal
 * @brief Formats ResourceChanged message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceChanged()
{
    return getLog(redfish::registries::resource_event::Index::resourceChanged,
                  {});
}

/**
 * @internal
 * @brief Formats ResourceVersionIncompatible message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceVersionIncompatible(std::string_view arg1)
{
    return getLog(
        redfish::registries::resource_event::Index::resourceVersionIncompatible,
        std::to_array({arg1}));
}

/**
 * @internal
 * @brief Formats ResourceSelfTestFailed message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceSelfTestFailed(std::string_view arg1)
{
    return getLog(
        redfish::registries::resource_event::Index::resourceSelfTestFailed,
        std::to_array({arg1}));
}

/**
 * @internal
 * @brief Formats ResourceSelfTestCompleted message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json resourceSelfTestCompleted()
{
    return getLog(
        redfish::registries::resource_event::Index::resourceSelfTestCompleted,
        {});
}

/**
 * @internal
 * @brief Formats TestMessage message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json testMessage()
{
    return getLog(redfish::registries::resource_event::Index::testMessage, {});
}

/**
 * @internal
 * @brief Formats AggregationSourceDiscovered message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json aggregationSourceDiscovered(std::string_view arg1,
                                           std::string_view arg2)
{
    return getLog(
        redfish::registries::resource_event::Index::aggregationSourceDiscovered,
        std::to_array({arg1, arg2}));
}

/**
 * @internal
 * @brief Formats LicenseExpired message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json licenseExpired(std::string_view arg1, std::string_view arg2)
{
    return getLog(redfish::registries::resource_event::Index::licenseExpired,
                  std::to_array({arg1, arg2}));
}

/**
 * @internal
 * @brief Formats LicenseChanged message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json licenseChanged(std::string_view arg1, std::string_view arg2)
{
    return getLog(redfish::registries::resource_event::Index::licenseChanged,
                  std::to_array({arg1, arg2}));
}

/**
 * @internal
 * @brief Formats LicenseAdded message into JSON
 *
 * See header file for more information
 * @endinternal
 */
nlohmann::json licenseAdded(std::string_view arg1, std::string_view arg2)
{
    return getLog(redfish::registries::resource_event::Index::licenseAdded,
                  std::to_array({arg1, arg2}));
}

} // namespace messages
} // namespace redfish
