#pragma once
#include <nlohmann/json.hpp>

namespace redfish_extensions
{
// clang-format off

enum class ReleaseStatusType{
    Invalid,
    Standard,
    Informational,
    WorkInProgress,
    InDevelopment,
};

enum class RevisionKind{
    Invalid,
    Added,
    Modified,
    Deprecated,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ReleaseStatusType, {
    {ReleaseStatusType::Invalid, "Invalid"},
    {ReleaseStatusType::Standard, "Standard"},
    {ReleaseStatusType::Informational, "Informational"},
    {ReleaseStatusType::WorkInProgress, "WorkInProgress"},
    {ReleaseStatusType::InDevelopment, "InDevelopment"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(RevisionKind, {
    {RevisionKind::Invalid, "Invalid"},
    {RevisionKind::Added, "Added"},
    {RevisionKind::Modified, "Modified"},
    {RevisionKind::Deprecated, "Deprecated"},
});

}
// clang-format on
