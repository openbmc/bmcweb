#pragma once
#include <nlohmann/json.hpp>

namespace job
{
// clang-format off

enum class JobState{
    Invalid,
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
};

NLOHMANN_JSON_SERIALIZE_ENUM(JobState, {
    {JobState::Invalid, "Invalid"},
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
});

}
// clang-format on
