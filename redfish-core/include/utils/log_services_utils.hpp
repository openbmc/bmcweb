// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2018 Intel Corporation

#pragma once

#include "bmcweb_config.h"

#include "async_resp.hpp"
#include "boost_formatters.hpp"
#include "error_messages.hpp"
#include "http_body.hpp"
#include "http_response.hpp"
#include "logging.hpp"

#include <asm-generic/errno.h>
#include <unistd.h>

#include <boost/beast/http/field.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <cstdio>
#include <string>
#include <string_view>

namespace redfish
{
namespace log_services_utils
{

enum class LogService
{
};

inline std::string logServiceToString(LogService logService)
{
    std::string serviceStr;
    switch (logService)
    {
        default:
            BMCWEB_LOG_ERROR("Unable to stringify bmcweb eventlog location");
            break;
    }

    return serviceStr;
}

constexpr const char* rfSystemsStr = "Systems";
constexpr const char* rfManagersStr = "Managers";

enum class LogServiceParentCollection
{
    Systems,
    Managers
};

inline std::string logServiceParentCollectionToString(
    LogServiceParentCollection collection)
{
    std::string collectionStr;
    switch (collection)
    {
        case LogServiceParentCollection::Managers:
            collectionStr = rfManagersStr;
            break;
        case LogServiceParentCollection::Systems:
            collectionStr = rfSystemsStr;
            break;
        default:
            BMCWEB_LOG_ERROR("Unable to stringify bmcweb eventlog location");
            break;
    }
    return collectionStr;
}

inline std::string_view getMemberIdFromParentCollection(
    LogServiceParentCollection collection)
{
    std::string_view memberId;

    switch (collection)
    {
        case LogServiceParentCollection::Managers:
            memberId = BMCWEB_REDFISH_MANAGER_URI_NAME;
            break;
        case LogServiceParentCollection::Systems:
            memberId = BMCWEB_REDFISH_SYSTEM_URI_NAME;
            break;
        default:
            BMCWEB_LOG_ERROR(
                "Unable to stringify bmcweb eventlog location childId");
            break;
    }
    return memberId;
}

inline std::string getLogEntryDescriptorFromParentCollection(
    LogServiceParentCollection collection)
{
    std::string descriptor;
    switch (collection)
    {
        case LogServiceParentCollection::Managers:
            descriptor = "Manager";
            break;
        case LogServiceParentCollection::Systems:
            descriptor = "System";
            break;
        default:
            BMCWEB_LOG_ERROR("Unable to get Log Entry descriptor");
            break;
    }
    return descriptor;
}

inline bool checkSizeLimit(int fd, crow::Response& res)
{
    long long int size = lseek(fd, 0, SEEK_END);
    if (size <= 0)
    {
        BMCWEB_LOG_ERROR("Failed to get size of file, lseek() returned {}",
                         size);
        messages::internalError(res);
        return false;
    }

    // Arbitrary max size of 20MB to accommodate BMC dumps
    constexpr long long int maxFileSize = 20LL * 1024LL * 1024LL;
    if (size > maxFileSize)
    {
        BMCWEB_LOG_ERROR("File size {} exceeds maximum allowed size of {}",
                         size, maxFileSize);
        messages::internalError(res);
        return false;
    }
    off_t rc = lseek(fd, 0, SEEK_SET);
    if (rc < 0)
    {
        BMCWEB_LOG_ERROR("Failed to reset file offset to 0");
        messages::internalError(res);
        return false;
    }
    return true;
}

inline void downloadEntryCallback(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& entryID, const std::string& downloadEntryType,
    const boost::system::error_code& ec,
    const sdbusplus::message::unix_fd& unixfd)
{
    if (ec.value() == EBADR)
    {
        messages::resourceNotFound(asyncResp->res, "EntryAttachment", entryID);
        return;
    }
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error: {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    // Make sure we know how to process the retrieved entry attachment
    if ((downloadEntryType != "BMC") && (downloadEntryType != "System"))
    {
        BMCWEB_LOG_ERROR("downloadEntryCallback() invalid entry type: {}",
                         downloadEntryType);
        messages::internalError(asyncResp->res);
    }

    int fd = -1;
    fd = dup(unixfd);
    if (fd < 0)
    {
        BMCWEB_LOG_ERROR("Failed to open file");
        messages::internalError(asyncResp->res);
        return;
    }
    if (!checkSizeLimit(fd, asyncResp->res))
    {
        close(fd);
        return;
    }
    if (downloadEntryType == "System")
    {
        if (!asyncResp->res.openFd(fd))
        {
            messages::internalError(asyncResp->res);
            close(fd);
            return;
        }
        asyncResp->res.addHeader(boost::beast::http::field::content_type,
                                 "application/json");
        return;
    }
    if (!asyncResp->res.openFd(fd))
    {
        messages::internalError(asyncResp->res);
        close(fd);
        return;
    }
    asyncResp->res.addHeader(boost::beast::http::field::content_type,
                             "application/octet-stream");
}
} // namespace log_services_utils
} // namespace redfish
