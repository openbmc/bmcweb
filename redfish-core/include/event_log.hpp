#pragma once

#include "event_logs_object_type.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace redfish
{

namespace event_log
{

bool getUniqueEntryID(const std::string& logEntry, std::string& entryID);

int getEventLogParams(const std::string& logEntry, std::string& timestamp,
                      std::string& messageID,
                      std::vector<std::string>& messageArgs);

void formatEventLogEntry(uint64_t eventId,
                         const redfish::EventLogObjectsType& logEntry,
                         const std::string& customText,
                         nlohmann::json::object_t& logEntryJson);

} // namespace event_log

} // namespace redfish
