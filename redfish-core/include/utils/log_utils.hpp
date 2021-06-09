#pragma once
#include <async_resp.hpp>

namespace redfish
{
namespace log_utils
{

// "/xyz/openbmc_project/logging/devices/" + storageId
// "OpenBmc.0.2.DriveError"
// auto logPath = "/redfish/v1/Chassis/" + storageId +
//    "/LogServices/DeviceLog/Entries/";

/**
 * @brief Populate the Status with device log entries
 *
 * @param[i,o] asyncResp   Async response object
 * @param[i]   logPath     Indicates what target to look for the logs
 * @param[i]   logURIBase  Base for the log URI
 * @param[i]   messageId   Message Id for the log entries
 * @param[i]   deviceId    Device Id to filter out the device log entries
 *
 * @return void
 */
inline void populateDeviceLogEntries(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& logPath, const std::string& logURIBase,
    const std::string& messageId, const std::string& deviceId)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, logURIBase, messageId,
         deviceId](const boost::system::error_code ec,
                   const crow::openbmc_mapper::GetSubTreeType& subtree) {
            if (ec)
            {
                return;
            }

            nlohmann::json& conditionArray =
                asyncResp->res.jsonValue["Status"]["Conditions"];
            conditionArray = nlohmann::json::array();
            for (const auto& obj : subtree)
            {
                sdbusplus::message::object_path objpath(obj.first);
                if (objpath.filename().empty())
                {
                    BMCWEB_LOG_ERROR << "Failed to find filename in "
                                     << obj.first;
                    continue;
                }
                const auto& entryName = objpath.filename();
                if (!boost::algorithm::starts_with(entryName, deviceId + "_"))
                {
                    continue;
                }

                conditionArray.push_back(nlohmann::json());
                auto& entry = conditionArray.back();

                entry["LogEntry"]["@odata.id"] = logURIBase + entryName;
                entry["MessageId"] = messageId;
            }

            asyncResp->res.jsonValue["Status"]["Conditions@odata.count"] =
                conditionArray.size();
        },
        // Use only the first service it finds.
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", logPath, int32_t(1),
        std::array<const char*, 1>{"xyz.openbmc_project.Logging.Entry"});
}

} // namespace log_utils
} // namespace redfish
