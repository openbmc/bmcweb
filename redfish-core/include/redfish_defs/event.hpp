#pragma once
#include <nlohmann/json.hpp>

namespace event
{
// clang-format off

enum class EventType{
    Invalid,
    StatusChange,
    ResourceUpdated,
    ResourceAdded,
    ResourceRemoved,
    Alert,
    MetricReport,
    Other,
};

NLOHMANN_JSON_SERIALIZE_ENUM(EventType, {
    {EventType::Invalid, "Invalid"},
    {EventType::StatusChange, "StatusChange"},
    {EventType::ResourceUpdated, "ResourceUpdated"},
    {EventType::ResourceAdded, "ResourceAdded"},
    {EventType::ResourceRemoved, "ResourceRemoved"},
    {EventType::Alert, "Alert"},
    {EventType::MetricReport, "MetricReport"},
    {EventType::Other, "Other"},
});

}
// clang-format on
