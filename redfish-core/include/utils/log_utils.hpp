#pragma once

#include <async_resp.hpp>

namespace redfish
{
namespace log_utils
{

/**
 * @brief Populate the Status with device log entries
 *
 * @param[i,o] asyncResp   Async response object
 * @param[i]   logPath     Dbus path to look for the logs
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
        [asyncResp, logPath, logURIBase, messageId,
         deviceId](const boost::system::error_code ec,
                   const crow::openbmc_mapper::GetSubTreeType& subtree) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << ec;
                return;
            }

            nlohmann::json& conditionArray =
                asyncResp->res.jsonValue["Status"]["Conditions"];
            conditionArray = nlohmann::json::array();
            for (const auto& obj : subtree)
            {
                // Remove the base log path to make sure the log service name
                // is not mistaken for device id.
                if (obj.first.size() <= logPath.size())
                {
                    BMCWEB_LOG_WARNING << "Failed to find log path in "
                                       << obj.first << ". Wants " << logPath;
                    continue;
                }

                // objpath should be /.../deviceId/entryId
                sdbusplus::message::object_path objpath(
                    obj.first.substr(logPath.size()));

                if (objpath.filename().empty())
                {
                    BMCWEB_LOG_WARNING << "Failed to find filename in "
                                       << obj.first;
                    continue;
                }

                const auto& entryName = objpath.filename();
                const auto& device = objpath.parent_path().filename();

                if (device != deviceId)
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
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", logPath, 0,
        std::array<const char*, 1>{"xyz.openbmc_project.Logging.Entry"});
}

} // namespace log_utils
} // namespace redfish
