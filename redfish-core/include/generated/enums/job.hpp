// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace job
{
// clang-format off

enum class JobState{
    New,
    Starting,
    Running,
    Suspended,
    Interrupted,
    Pending,
    Stopping,
    Completed,
    Cancelled,
    Exception,
    Service,
    UserIntervention,
    Continue,
    Validating,
    Invalid,
};

enum class JobType{
    Invalid,
    DocumentBased,
    UserSpecified,
    ServiceGenerated,
};

NLOHMANN_JSON_SERIALIZE_ENUM(JobState, {
    {JobState::New, "New"},
    {JobState::Starting, "Starting"},
    {JobState::Running, "Running"},
    {JobState::Suspended, "Suspended"},
    {JobState::Interrupted, "Interrupted"},
    {JobState::Pending, "Pending"},
    {JobState::Stopping, "Stopping"},
    {JobState::Completed, "Completed"},
    {JobState::Cancelled, "Cancelled"},
    {JobState::Exception, "Exception"},
    {JobState::Service, "Service"},
    {JobState::UserIntervention, "UserIntervention"},
    {JobState::Continue, "Continue"},
    {JobState::Validating, "Validating"},
    {JobState::Invalid, "Invalid"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(JobType, {
    {JobType::Invalid, "Invalid"},
    {JobType::DocumentBased, "DocumentBased"},
    {JobType::UserSpecified, "UserSpecified"},
    {JobType::ServiceGenerated, "ServiceGenerated"},
});

}
// clang-format on
