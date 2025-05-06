#pragma once
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
// These generated headers are a superset of what is needed.
// clang sees them as an error, so ignore
// NOLINTBEGIN(misc-include-cleaner)
#include "http_response.hpp"

#include <boost/url/url_view_base.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <source_location>
#include <string_view>
// NOLINTEND(misc-include-cleaner)

namespace redfish
{

namespace messages
{
nlohmann::json::object_t resourceCreated();

nlohmann::json::object_t resourceRemoved();

nlohmann::json::object_t resourceErrorsDetected(std::string_view arg1,
                                                std::string_view arg2);

nlohmann::json::object_t resourceErrorsCorrected(std::string_view arg1,
                                                 std::string_view arg2);

nlohmann::json::object_t resourceErrorThresholdExceeded(std::string_view arg1,
                                                        std::string_view arg2);

nlohmann::json::object_t resourceErrorThresholdCleared(std::string_view arg1,
                                                       std::string_view arg2);

nlohmann::json::object_t resourceWarningThresholdExceeded(
    std::string_view arg1, std::string_view arg2);

nlohmann::json::object_t resourceWarningThresholdCleared(std::string_view arg1,
                                                         std::string_view arg2);

nlohmann::json::object_t resourceStatusChangedOK(std::string_view arg1,
                                                 std::string_view arg2);

nlohmann::json::object_t resourceStatusChangedWarning(std::string_view arg1,
                                                      std::string_view arg2);

nlohmann::json::object_t resourceStatusChangedCritical(std::string_view arg1,
                                                       std::string_view arg2);

nlohmann::json::object_t resourceStateChanged(std::string_view arg1,
                                              std::string_view arg2);

nlohmann::json::object_t resourcePoweredOn(std::string_view arg1);

nlohmann::json::object_t resourcePoweringOn(std::string_view arg1);

nlohmann::json::object_t resourcePoweredOff(std::string_view arg1);

nlohmann::json::object_t resourcePoweringOff(std::string_view arg1);

nlohmann::json::object_t resourcePaused(std::string_view arg1);

nlohmann::json::object_t uRIForResourceChanged();

nlohmann::json::object_t resourceChanged();

nlohmann::json::object_t resourceVersionIncompatible(std::string_view arg1);

nlohmann::json::object_t resourceSelfTestFailed(std::string_view arg1);

nlohmann::json::object_t resourceSelfTestCompleted();

nlohmann::json::object_t testMessage();

nlohmann::json::object_t aggregationSourceDiscovered(std::string_view arg1,
                                                     std::string_view arg2);

nlohmann::json::object_t licenseExpired(std::string_view arg1,
                                        std::string_view arg2);

nlohmann::json::object_t licenseChanged(std::string_view arg1,
                                        std::string_view arg2);

nlohmann::json::object_t licenseAdded(std::string_view arg1,
                                      std::string_view arg2);

} // namespace messages
} // namespace redfish
