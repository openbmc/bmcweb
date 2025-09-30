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
nlohmann::json::object_t targetDetermined(std::string_view arg1,
                                          std::string_view arg2);

nlohmann::json::object_t allTargetsDetermined();

nlohmann::json::object_t noTargetsDetermined(std::string_view arg1);

nlohmann::json::object_t updateInProgress();

nlohmann::json::object_t transferringToComponent(std::string_view arg1,
                                                 std::string_view arg2);

nlohmann::json::object_t verifyingAtComponent(std::string_view arg1,
                                              std::string_view arg2);

nlohmann::json::object_t installingOnComponent(std::string_view arg1,
                                               std::string_view arg2);

nlohmann::json::object_t applyingOnComponent(std::string_view arg1,
                                             std::string_view arg2);

nlohmann::json::object_t transferFailed(std::string_view arg1,
                                        std::string_view arg2);

nlohmann::json::object_t verificationFailed(std::string_view arg1,
                                            std::string_view arg2);

nlohmann::json::object_t applyFailed(std::string_view arg1,
                                     std::string_view arg2);

nlohmann::json::object_t activateFailed(std::string_view arg1,
                                        std::string_view arg2);

nlohmann::json::object_t awaitToUpdate(std::string_view arg1,
                                       std::string_view arg2);

nlohmann::json::object_t awaitToActivate(std::string_view arg1,
                                         std::string_view arg2);

nlohmann::json::object_t updateSuccessful(std::string_view arg1,
                                          std::string_view arg2);

nlohmann::json::object_t operationTransitionedToJob(std::string_view arg1);

nlohmann::json::object_t updateSkipped(std::string_view arg1,
                                       std::string_view arg2);

nlohmann::json::object_t updateSkippedSameVersion(std::string_view arg1,
                                                  std::string_view arg2);

nlohmann::json::object_t updateNotApplicable(std::string_view arg1,
                                             std::string_view arg2);

} // namespace messages
} // namespace redfish
