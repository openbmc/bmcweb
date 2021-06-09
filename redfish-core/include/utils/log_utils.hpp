#pragma once

#include <async_resp.hpp>
#include <sdbusplus/asio/property.hpp>

namespace redfish
{
namespace log_utils
{

/**
 * @brief Populate the Status with device log entries
 *
 * @param[i,o] asyncResp   Async response object
 * @param[i]   jsonPtr     Json path to save the LogEntires
 * @param[i]   path        Dbus path of the resource
 *
 * @return void
 */
inline void getLogEntry(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const nlohmann::json_pointer<nlohmann::json>& jsonPtr,
                        const std::string& path, const std::string& messageId)
{
    sdbusplus::asio::getProperty<std::vector<std::string>>(
        *crow::connections::systemBus, "xyz.openbmc_project.ObjectMapper",
        path + "/Chassis", "xyz.openbmc_project.Association", "endpoints",
        [asyncResp, jsonPtr, path,
         messageId](const boost::system::error_code ec,
                    const std::vector<std::string>& chassisList) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG
                    << "Failed to get Chassis Association: " << ec + << " with "
                    << path + "/Chassis";
                return;
            }

            if (chassisList.size() != 1)
            {
                BMCWEB_LOG_DEBUG
                    << "Resource can only be included in one chassis: " << path;
                return;
            }
            const std::string& chassisId =
                sdbusplus::message::object_path(chassisList[0]).filename();
            sdbusplus::asio::getProperty<std::vector<std::string>>(
                *crow::connections::systemBus,
                "xyz.openbmc_project.ObjectMapper", path + "/DeviceLog",
                "xyz.openbmc_project.Association", "endpoints",
                [asyncResp, jsonPtr, chassisId, path,
                 messageId](const boost::system::error_code ec,
                            const std::vector<std::string>& logPathList) {
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG << "Failed to get Log Association: "
                                         << ec;
                        return;
                    }
                    if (logPathList.empty())
                    {
                        BMCWEB_LOG_DEBUG
                            << "Resource does not have any log avaliable: "
                            << path;
                        return;
                    }

                    asyncResp->res
                        .jsonValue["Status"]["Conditions@odata.count"] =
                        conditionArray.size();
                    nlohmann::json& conditionArray =
                        asyncResp->res.jsonValue[jsonPtr]["Conditions"];
                    conditionArray = nlohmann::json::array();
                    for (const auto& logPath : logPathList)
                    {
                        sdbusplus::message::object_path objPath(logPath);
                        conditionArray.push_back(nlohmann::json());
                        auto& entry = conditionArray.back();
                        entry["LogEntry"]["@odata.id"] =
                            "/redfish/v1/Chassis/" + chassisId +
                            "/LogServices/DeviceLog/Entries/" +
                            objPath.filename();
                        entry["MessageId"] = messageId;
                    }
                    asyncResp->res
                        .jsonValue[jsonPtr]["Conditions@odata.count"] =
                        conditionArray.size();
                });
        });
}

} // namespace log_utils
} // namespace redfish
