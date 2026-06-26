// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace redfish_extensions
{
// clang-format off

enum class ClientContextStatusType{
    Applied,
    Invalid,
    NotFound,
    Unsupported,
};

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

NLOHMANN_JSON_SERIALIZE_ENUM(ClientContextStatusType, {
    {ClientContextStatusType::Applied, "Applied"},
    {ClientContextStatusType::Invalid, "Invalid"},
    {ClientContextStatusType::NotFound, "NotFound"},
    {ClientContextStatusType::Unsupported, "Unsupported"},
});

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

// clang-format on
} // namespace redfish_extensions
