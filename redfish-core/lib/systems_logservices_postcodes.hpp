#pragma once

#include "app.hpp"
#include "generated/enums/log_service.hpp"
#include "query.hpp"
#include "registries/openbmc_message_registry.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/time_utils.hpp"

#include <cstdint>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace redfish
{

inline void handleSystemsLogServicesPostCodesGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName)
{
    if (!setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    asyncResp->res.jsonValue["@odata.id"] =
        std::format("/redfish/v1/Systems/{}/LogServices/PostCodes",
                    BMCWEB_REDFISH_SYSTEM_URI_NAME);
    asyncResp->res.jsonValue["@odata.type"] = "#LogService.v1_2_0.LogService";
    asyncResp->res.jsonValue["Name"] = "POST Code Log Service";
    asyncResp->res.jsonValue["Description"] = "POST Code Log Service";
    asyncResp->res.jsonValue["Id"] = "PostCodes";
    asyncResp->res.jsonValue["OverWritePolicy"] =
        log_service::OverWritePolicy::WrapsWhenFull;
    asyncResp->res.jsonValue["Entries"]["@odata.id"] =
        std::format("/redfish/v1/Systems/{}/LogServices/PostCodes/Entries",
                    BMCWEB_REDFISH_SYSTEM_URI_NAME);

    std::pair<std::string, std::string> redfishDateTimeOffset =
        time_utils::getDateTimeOffsetNow();
    asyncResp->res.jsonValue["DateTime"] = redfishDateTimeOffset.first;
    asyncResp->res.jsonValue["DateTimeLocalOffset"] =
        redfishDateTimeOffset.second;

    asyncResp->res
        .jsonValue["Actions"]["#LogService.ClearLog"]["target"] = std::format(
        "/redfish/v1/Systems/{}/LogServices/PostCodes/Actions/LogService.ClearLog",
        BMCWEB_REDFISH_SYSTEM_URI_NAME);
}

inline void handleSystemsLogServicesPostCodesPost(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName)
{
    if (!setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    BMCWEB_LOG_DEBUG("Do delete all postcodes entries.");

    // Make call to post-code service to request clear all
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code& ec) {
            if (ec)
            {
                // TODO Handle for specific error code
                BMCWEB_LOG_ERROR("doClearPostCodes resp_handler got error {}",
                                 ec);
                asyncResp->res.result(
                    boost::beast::http::status::internal_server_error);
                messages::internalError(asyncResp->res);
                return;
            }
            messages::success(asyncResp->res);
        },
        "xyz.openbmc_project.State.Boot.PostCode0",
        "/xyz/openbmc_project/State/Boot/PostCode0",
        "xyz.openbmc_project.Collection.DeleteAll", "DeleteAll");
}

/**
 * @brief Parse post code ID and get the current value and index value
 *        eg: postCodeID=B1-2, currentValue=1, index=2
 *
 * @param[in]  postCodeID     Post Code ID
 * @param[out] currentValue   Current value
 * @param[out] index          Index value
 *
 * @return bool true if the parsing is successful, false the parsing fails
 */
inline bool parsePostCode(std::string_view postCodeID, uint64_t& currentValue,
                          uint16_t& index)
{
    std::vector<std::string> split;
    bmcweb::split(split, postCodeID, '-');
    if (split.size() != 2)
    {
        return false;
    }
    std::string_view postCodeNumber = split[0];
    if (postCodeNumber.size() < 2)
    {
        return false;
    }
    if (postCodeNumber[0] != 'B')
    {
        return false;
    }
    postCodeNumber.remove_prefix(1);
    auto [ptrIndex, ecIndex] =
        std::from_chars(postCodeNumber.begin(), postCodeNumber.end(), index);
    if (ptrIndex != postCodeNumber.end() || ecIndex != std::errc())
    {
        return false;
    }

    std::string_view postCodeIndex = split[1];

    auto [ptrValue, ecValue] = std::from_chars(
        postCodeIndex.begin(), postCodeIndex.end(), currentValue);

    return ptrValue == postCodeIndex.end() && ecValue == std::errc();
}

static bool fillPostCodeEntry(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::container::flat_map<
        uint64_t, std::tuple<uint64_t, std::vector<uint8_t>>>& postcode,
    const uint16_t bootIndex, const uint64_t codeIndex = 0,
    const uint64_t skip = 0, const uint64_t top = 0)
{
    // Get the Message from the MessageRegistry
    const registries::Message* message =
        registries::getMessage("OpenBMC.0.2.BIOSPOSTCode");
    if (message == nullptr)
    {
        BMCWEB_LOG_ERROR("Couldn't find known message?");
        return false;
    }
    uint64_t currentCodeIndex = 0;
    uint64_t firstCodeTimeUs = 0;
    for (const std::pair<uint64_t, std::tuple<uint64_t, std::vector<uint8_t>>>&
             code : postcode)
    {
        currentCodeIndex++;
        std::string postcodeEntryID =
            "B" + std::to_string(bootIndex) + "-" +
            std::to_string(currentCodeIndex); // 1 based index in EntryID string

        uint64_t usecSinceEpoch = code.first;
        uint64_t usTimeOffset = 0;

        if (1 == currentCodeIndex)
        { // already incremented
            firstCodeTimeUs = code.first;
        }
        else
        {
            usTimeOffset = code.first - firstCodeTimeUs;
        }

        // skip if no specific codeIndex is specified and currentCodeIndex does
        // not fall between top and skip
        if ((codeIndex == 0) &&
            (currentCodeIndex <= skip || currentCodeIndex > top))
        {
            continue;
        }

        // skip if a specific codeIndex is specified and does not match the
        // currentIndex
        if ((codeIndex > 0) && (currentCodeIndex != codeIndex))
        {
            // This is done for simplicity. 1st entry is needed to calculate
            // time offset. To improve efficiency, one can get to the entry
            // directly (possibly with flatmap's nth method)
            continue;
        }

        // currentCodeIndex is within top and skip or equal to specified code
        // index

        // Get the Created time from the timestamp
        std::string entryTimeStr;
        entryTimeStr = time_utils::getDateTimeUintUs(usecSinceEpoch);

        // assemble messageArgs: BootIndex, TimeOffset(100us), PostCode(hex)
        std::ostringstream hexCode;
        hexCode << "0x" << std::setfill('0') << std::setw(2) << std::hex
                << std::get<0>(code.second);
        std::ostringstream timeOffsetStr;
        // Set Fixed -Point Notation
        timeOffsetStr << std::fixed;
        // Set precision to 4 digits
        timeOffsetStr << std::setprecision(4);
        // Add double to stream
        timeOffsetStr << static_cast<double>(usTimeOffset) / 1000 / 1000;

        std::string bootIndexStr = std::to_string(bootIndex);
        std::string timeOffsetString = timeOffsetStr.str();
        std::string hexCodeStr = hexCode.str();

        std::array<std::string_view, 3> messageArgs = {
            bootIndexStr, timeOffsetString, hexCodeStr};

        std::string msg =
            registries::fillMessageArgs(messageArgs, message->message);
        if (msg.empty())
        {
            messages::internalError(asyncResp->res);
            return false;
        }

        // Get Severity template from message registry
        std::string severity;
        if (message != nullptr)
        {
            severity = message->messageSeverity;
        }

        // Format entry
        nlohmann::json::object_t bmcLogEntry;
        bmcLogEntry["@odata.type"] = "#LogEntry.v1_9_0.LogEntry";
        bmcLogEntry["@odata.id"] = boost::urls::format(
            "/redfish/v1/Systems/{}/LogServices/PostCodes/Entries/{}",
            BMCWEB_REDFISH_SYSTEM_URI_NAME, postcodeEntryID);
        bmcLogEntry["Name"] = "POST Code Log Entry";
        bmcLogEntry["Id"] = postcodeEntryID;
        bmcLogEntry["Message"] = std::move(msg);
        bmcLogEntry["MessageId"] = "OpenBMC.0.2.BIOSPOSTCode";
        bmcLogEntry["MessageArgs"] = messageArgs;
        bmcLogEntry["EntryType"] = "Event";
        bmcLogEntry["Severity"] = std::move(severity);
        bmcLogEntry["Created"] = entryTimeStr;
        if (!std::get<std::vector<uint8_t>>(code.second).empty())
        {
            bmcLogEntry["AdditionalDataURI"] =
                std::format(
                    "/redfish/v1/Systems/{}/LogServices/PostCodes/Entries/",
                    BMCWEB_REDFISH_SYSTEM_URI_NAME) +
                postcodeEntryID + "/attachment";
        }

        // codeIndex is only specified when querying single entry, return only
        // that entry in this case
        if (codeIndex != 0)
        {
            asyncResp->res.jsonValue.update(bmcLogEntry);
            return true;
        }

        nlohmann::json& logEntryArray = asyncResp->res.jsonValue["Members"];
        logEntryArray.emplace_back(std::move(bmcLogEntry));
    }

    // Return value is always false when querying multiple entries
    return false;
}

inline void
    getPostCodeForEntry(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& entryId)
{
    uint16_t bootIndex = 0;
    uint64_t codeIndex = 0;
    if (!parsePostCode(entryId, codeIndex, bootIndex))
    {
        // Requested ID was not found
        messages::resourceNotFound(asyncResp->res, "LogEntry", entryId);
        return;
    }

    if (bootIndex == 0 || codeIndex == 0)
    {
        // 0 is an invalid index
        messages::resourceNotFound(asyncResp->res, "LogEntry", entryId);
        return;
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp, entryId, bootIndex,
         codeIndex](const boost::system::error_code& ec,
                    const boost::container::flat_map<
                        uint64_t, std::tuple<uint64_t, std::vector<uint8_t>>>&
                        postcode) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG("DBUS POST CODE PostCode response error");
                messages::internalError(asyncResp->res);
                return;
            }

            if (postcode.empty())
            {
                messages::resourceNotFound(asyncResp->res, "LogEntry", entryId);
                return;
            }

            if (!fillPostCodeEntry(asyncResp, postcode, bootIndex, codeIndex))
            {
                messages::resourceNotFound(asyncResp->res, "LogEntry", entryId);
                return;
            }
        },
        "xyz.openbmc_project.State.Boot.PostCode0",
        "/xyz/openbmc_project/State/Boot/PostCode0",
        "xyz.openbmc_project.State.Boot.PostCode", "GetPostCodesWithTimeStamp",
        bootIndex);
}

inline void
    getPostCodeForBoot(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const uint16_t bootIndex, const uint16_t bootCount,
                       const uint64_t entryCount, size_t skip, size_t top)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, bootIndex, bootCount, entryCount, skip,
         top](const boost::system::error_code& ec,
              const boost::container::flat_map<
                  uint64_t, std::tuple<uint64_t, std::vector<uint8_t>>>&
                  postcode) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG("DBUS POST CODE PostCode response error");
                messages::internalError(asyncResp->res);
                return;
            }

            uint64_t endCount = entryCount;
            if (!postcode.empty())
            {
                endCount = entryCount + postcode.size();
                if (skip < endCount && (top + skip) > entryCount)
                {
                    uint64_t thisBootSkip =
                        std::max(static_cast<uint64_t>(skip), entryCount) -
                        entryCount;
                    uint64_t thisBootTop =
                        std::min(static_cast<uint64_t>(top + skip), endCount) -
                        entryCount;

                    fillPostCodeEntry(asyncResp, postcode, bootIndex, 0,
                                      thisBootSkip, thisBootTop);
                }
                asyncResp->res.jsonValue["Members@odata.count"] = endCount;
            }

            // continue to previous bootIndex
            if (bootIndex < bootCount)
            {
                getPostCodeForBoot(asyncResp,
                                   static_cast<uint16_t>(bootIndex + 1),
                                   bootCount, endCount, skip, top);
            }
            else if (skip + top < endCount)
            {
                asyncResp->res.jsonValue["Members@odata.nextLink"] =
                    std::format(
                        "/redfish/v1/Systems/{}/LogServices/PostCodes/Entries?$skip=",
                        BMCWEB_REDFISH_SYSTEM_URI_NAME) +
                    std::to_string(skip + top);
            }
        },
        "xyz.openbmc_project.State.Boot.PostCode0",
        "/xyz/openbmc_project/State/Boot/PostCode0",
        "xyz.openbmc_project.State.Boot.PostCode", "GetPostCodesWithTimeStamp",
        bootIndex);
}

inline void
    getCurrentBootNumber(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         size_t skip, size_t top)
{
    uint64_t entryCount = 0;
    sdbusplus::asio::getProperty<uint16_t>(
        *crow::connections::systemBus,
        "xyz.openbmc_project.State.Boot.PostCode0",
        "/xyz/openbmc_project/State/Boot/PostCode0",
        "xyz.openbmc_project.State.Boot.PostCode", "CurrentBootCycleCount",
        [asyncResp, entryCount, skip,
         top](const boost::system::error_code& ec, const uint16_t bootCount) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG("DBUS response error {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }
            getPostCodeForBoot(asyncResp, 1, bootCount, entryCount, skip, top);
        });
}

inline void handleSystemsLogServicesPostCodesEntriesGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName)
{
    query_param::QueryCapabilities capabilities = {
        .canDelegateTop = true,
        .canDelegateSkip = true,
    };
    query_param::Query delegatedQuery;
    if (!setUpRedfishRouteWithDelegation(app, req, asyncResp, delegatedQuery,
                                         capabilities))
    {
        return;
    }
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    asyncResp->res.jsonValue["@odata.type"] =
        "#LogEntryCollection.LogEntryCollection";
    asyncResp->res.jsonValue["@odata.id"] =
        std::format("/redfish/v1/Systems/{}/LogServices/PostCodes/Entries",
                    BMCWEB_REDFISH_SYSTEM_URI_NAME);
    asyncResp->res.jsonValue["Name"] = "BIOS POST Code Log Entries";
    asyncResp->res.jsonValue["Description"] =
        "Collection of POST Code Log Entries";
    asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
    asyncResp->res.jsonValue["Members@odata.count"] = 0;
    size_t skip = delegatedQuery.skip.value_or(0);
    size_t top = delegatedQuery.top.value_or(query_param::Query::maxTop);
    getCurrentBootNumber(asyncResp, skip, top);
}

inline void handleSystemsLogServicesPostCodesEntriesEntryAdditionalDataGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& postCodeID)
{
    if (!setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if (!http_helpers::isContentTypeAllowed(
            req.getHeaderValue("Accept"),
            http_helpers::ContentType::OctetStream, true))
    {
        asyncResp->res.result(boost::beast::http::status::bad_request);
        return;
    }
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    uint64_t currentValue = 0;
    uint16_t index = 0;
    if (!parsePostCode(postCodeID, currentValue, index))
    {
        messages::resourceNotFound(asyncResp->res, "LogEntry", postCodeID);
        return;
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp, postCodeID, currentValue](
            const boost::system::error_code& ec,
            const std::vector<std::tuple<uint64_t, std::vector<uint8_t>>>&
                postcodes) {
            if (ec.value() == EBADR)
            {
                messages::resourceNotFound(asyncResp->res, "LogEntry",
                                           postCodeID);
                return;
            }
            if (ec)
            {
                BMCWEB_LOG_DEBUG("DBUS response error {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }

            size_t value = static_cast<size_t>(currentValue) - 1;
            if (value == std::string::npos || postcodes.size() < currentValue)
            {
                BMCWEB_LOG_WARNING("Wrong currentValue value");
                messages::resourceNotFound(asyncResp->res, "LogEntry",
                                           postCodeID);
                return;
            }

            const auto& [tID, c] = postcodes[value];
            if (c.empty())
            {
                BMCWEB_LOG_WARNING("No found post code data");
                messages::resourceNotFound(asyncResp->res, "LogEntry",
                                           postCodeID);
                return;
            }
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            const char* d = reinterpret_cast<const char*>(c.data());
            std::string_view strData(d, c.size());

            asyncResp->res.addHeader(boost::beast::http::field::content_type,
                                     "application/octet-stream");
            asyncResp->res.addHeader(
                boost::beast::http::field::content_transfer_encoding, "Base64");
            asyncResp->res.write(crow::utility::base64encode(strData));
        },
        "xyz.openbmc_project.State.Boot.PostCode0",
        "/xyz/openbmc_project/State/Boot/PostCode0",
        "xyz.openbmc_project.State.Boot.PostCode", "GetPostCodes", index);
}

inline void handleSystemsLogServicesPostCodesEntriesEntryGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& targetID)
{
    if (!setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    getPostCodeForEntry(asyncResp, targetID);
}

inline void requestRoutesSystemsLogServicesPostCode(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/LogServices/PostCodes/")
        .privileges(privileges::getLogService)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleSystemsLogServicesPostCodesGet, std::ref(app)));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/LogServices/PostCodes/Actions/LogService.ClearLog/")
        // The following privilege is correct; we need "SubordinateOverrides"
        // before we can automate it.
        .privileges({{"ConfigureComponents"}})
        .methods(boost::beast::http::verb::post)(std::bind_front(
            handleSystemsLogServicesPostCodesPost, std::ref(app)));

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/<str>/LogServices/PostCodes/Entries/")
        .privileges(privileges::getLogEntryCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleSystemsLogServicesPostCodesEntriesGet, std::ref(app)));

    BMCWEB_ROUTE(
        app, "/redfish/v1/Systems/<str>/LogServices/PostCodes/Entries/<str>/")
        .privileges(privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleSystemsLogServicesPostCodesEntriesEntryGet, std::ref(app)));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/LogServices/PostCodes/Entries/<str>/attachment/")
        .privileges(privileges::getLogEntry)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleSystemsLogServicesPostCodesEntriesEntryAdditionalDataGet,
            std::ref(app)));
}
} // namespace redfish
