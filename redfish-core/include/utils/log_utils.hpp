#pragma once

#include <async_resp.hpp>
#include <sdbusplus/asio/property.hpp>

namespace redfish
{
namespace log_utils
{

/**
 * @brief Populate the Status with Chassis log entries
 *
 * @param[i,o] asyncResp   Async response object
 * @param[i]   jsonPtr     Json path to save the LogEntires
 * @param[i]   path        Dbus path of the resource
 *
 * @return void
 */
inline void
    getChassisLogEntry(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const nlohmann::json::json_pointer& jsonPtr,
                       const std::string& path, const std::string& messageId)
{
    sdbusplus::asio::getProperty<std::vector<std::string>>(
        *crow::connections::systemBus, "xyz.openbmc_project.ObjectMapper",
        path + "/chassis", "xyz.openbmc_project.Association", "endpoints",
        [asyncResp, jsonPtr, path,
         messageId](const boost::system::error_code ec,
                    const std::vector<std::string>& chassisList) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "Failed to get Chassis Association: " << ec
                             << " with " << path + "/chassis";
            return;
        }

        if (chassisList.size() != 1)
        {
            BMCWEB_LOG_DEBUG << "Resource can only be included in one chassis: "
                             << path;
            return;
        }
        const std::string& chassisId =
            sdbusplus::message::object_path(chassisList[0]).filename();
        sdbusplus::asio::getProperty<std::vector<std::string>>(
            *crow::connections::systemBus, "xyz.openbmc_project.ObjectMapper",
            path + "/log_entries", "xyz.openbmc_project.Association",
            "endpoints",
            [asyncResp, jsonPtr, chassisId, path,
             messageId](const boost::system::error_code ec2,
                        const std::vector<std::string>& logPathList) {
            if (ec2)
            {
                BMCWEB_LOG_DEBUG << "Failed to get Log Association: " << ec2;
                return;
            }
            if (logPathList.empty())
            {
                BMCWEB_LOG_DEBUG << "Resource does not have any log avaliable: "
                                 << path;
                return;
            }

            nlohmann::json::array_t conditions;
            for (const auto& logPath : logPathList)
            {
                nlohmann::json::object_t entry;
                entry["LogEntry"]["@odata.id"] = crow::utility::urlFromPieces(
                    "redfish", "v1", "Chassis", chassisId, "LogServices",
                    "ChassisLog", "Entries",
                    sdbusplus::message::object_path(logPath).filename());
                entry["MessageId"] = messageId;
                conditions.push_back(std::move(entry));
            }
            asyncResp->res.jsonValue[jsonPtr]["Conditions@odata.count"] =
                conditions.size();
            asyncResp->res.jsonValue[jsonPtr]["Conditions"] =
                std::move(conditions);
            });
        });
}

} // namespace log_utils
} // namespace redfish
