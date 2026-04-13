#pragma once

#include "utils/time_utils.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace redfish
{

namespace event_log
{

struct UniqueEntryIDState
{
    using time_point = std::chrono::time_point<std::chrono::system_clock,
                                               std::chrono::seconds>;
    time_point prevTs = time_point::min();
    int index = 0;
};

bool getUniqueEntryID(UniqueEntryIDState& state, const std::string& logEntry,
                      std::string& entryID);

int getEventLogParams(const std::string& logEntry, std::string& timestamp,
                      std::string& messageID,
                      std::vector<std::string>& messageArgs);

int formatEventLogEntry(uint64_t eventId, const std::string& logEntryID,
                        const std::string& messageID,
                        std::span<std::string_view> messageArgs,
                        std::string timestamp, const std::string& customText,
                        nlohmann::json::object_t& logEntryJson);

} // namespace event_log

} // namespace redfish
