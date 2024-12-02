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

#include "http_response.hpp"

#include <boost/url/url_view_base.hpp>
#include <nlohmann/json.hpp>

#include <source_location>
#include <string_view>

// IWYU pragma: no_forward_declare crow::Response

namespace redfish
{

namespace messages
{
nlohmann::json resourceCreated();

nlohmann::json resourceRemoved();

nlohmann::json resourceErrorsDetected(std::string_view arg1,
                                      std::string_view arg2);

nlohmann::json resourceErrorsCorrected(std::string_view arg1,
                                       std::string_view arg2);

nlohmann::json resourceErrorThresholdExceeded(std::string_view arg1,
                                              std::string_view arg2);

nlohmann::json
    resourceErrorThresholdCleared(std::string_view arg1, std::string_view arg2);

nlohmann::json resourceWarningThresholdExceeded(std::string_view arg1,
                                                std::string_view arg2);

nlohmann::json resourceWarningThresholdCleared(std::string_view arg1,
                                               std::string_view arg2);

nlohmann::json resourceStatusChangedOK(std::string_view arg1,
                                       std::string_view arg2);

nlohmann::json
    resourceStatusChangedWarning(std::string_view arg1, std::string_view arg2);

nlohmann::json
    resourceStatusChangedCritical(std::string_view arg1, std::string_view arg2);

nlohmann::json resourceStateChanged(std::string_view arg1,
                                    std::string_view arg2);

nlohmann::json resourcePoweredOn(std::string_view arg1);

nlohmann::json resourcePoweringOn(std::string_view arg1);

nlohmann::json resourcePoweredOff(std::string_view arg1);

nlohmann::json resourcePoweringOff(std::string_view arg1);

nlohmann::json resourcePaused(std::string_view arg1);

nlohmann::json uRIForResourceChanged();

nlohmann::json resourceChanged();

nlohmann::json resourceVersionIncompatible(std::string_view arg1);

nlohmann::json resourceSelfTestFailed(std::string_view arg1);

nlohmann::json resourceSelfTestCompleted();

nlohmann::json testMessage();

nlohmann::json aggregationSourceDiscovered(std::string_view arg1,
                                           std::string_view arg2);

nlohmann::json licenseExpired(std::string_view arg1, std::string_view arg2);

nlohmann::json licenseChanged(std::string_view arg1, std::string_view arg2);

nlohmann::json licenseAdded(std::string_view arg1, std::string_view arg2);

} // namespace messages
} // namespace redfish
