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

#include <cstdint>
#include <source_location>
#include <string>
#include <string_view>

// IWYU pragma: no_forward_declare crow::Response

namespace redfish
{

namespace messages
{
nlohmann::json taskStarted(std::string_view arg1);

nlohmann::json taskCompletedOK(std::string_view arg1);

nlohmann::json taskCompletedWarning(std::string_view arg1);

nlohmann::json taskAborted(std::string_view arg1);

nlohmann::json taskCancelled(std::string_view arg1);

nlohmann::json taskRemoved(std::string_view arg1);

nlohmann::json taskPaused(std::string_view arg1);

nlohmann::json taskResumed(std::string_view arg1);

nlohmann::json taskProgressChanged(std::string_view arg1, uint64_t arg2);

} // namespace messages
} // namespace redfish
