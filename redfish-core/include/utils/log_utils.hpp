// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "logging.hpp"
#include "registries.hpp"

#include <nlohmann/json.hpp>

#include <span>
#include <string>

namespace redfish
{

namespace log_utils
{
// Fill json object with common fields for LogEntry and
// Event.EventRecord structures, callers are supposed to fill out
// the rest
inline bool fillLogObjPartially(
    const std::string& logEntryID, const std::string& messageID,
    const std::span<std::string_view> messageArgs, bool fillResolution,
    nlohmann::json& objectToFillOut)
{
    // Get the Message from the MessageRegistry
    const registries::Message* message = registries::getMessage(messageID);
    if (message == nullptr)
    {
        BMCWEB_LOG_DEBUG(
            "{}: could not find messageID '{}' for log entry {} in registry",
            __func__, messageID, logEntryID);
        return false;
    }

    std::string msg =
        redfish::registries::fillMessageArgs(messageArgs, message->message);
    if (msg.empty())
    {
        BMCWEB_LOG_DEBUG(
            "{}: message is empty after filling fillMessageArgs for log entry {}",
            __func__, logEntryID);
        return false;
    }

    objectToFillOut["Severity"] = message->messageSeverity;
    objectToFillOut["Message"] = std::move(msg);
    objectToFillOut["MessageId"] = messageID;
    objectToFillOut["MessageArgs"] = messageArgs;
    if (fillResolution)
    {
        objectToFillOut["Resolution"] = message->resolution;
    }
    return true;
}
} // namespace log_utils
} // namespace redfish
