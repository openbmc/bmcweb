#pragma once
#include <nlohmann/json.hpp>

namespace task
{
// clang-format off

enum class TaskState{
    Invalid,
    New,
    Starting,
    Running,
    Suspended,
    Interrupted,
    Pending,
    Stopping,
    Completed,
    Killed,
    Exception,
    Service,
    Cancelling,
    Cancelled,
};

NLOHMANN_JSON_SERIALIZE_ENUM(TaskState, {
    {TaskState::Invalid, "Invalid"},
    {TaskState::New, "New"},
    {TaskState::Starting, "Starting"},
    {TaskState::Running, "Running"},
    {TaskState::Suspended, "Suspended"},
    {TaskState::Interrupted, "Interrupted"},
    {TaskState::Pending, "Pending"},
    {TaskState::Stopping, "Stopping"},
    {TaskState::Completed, "Completed"},
    {TaskState::Killed, "Killed"},
    {TaskState::Exception, "Exception"},
    {TaskState::Service, "Service"},
    {TaskState::Cancelling, "Cancelling"},
    {TaskState::Cancelled, "Cancelled"},
});

}
// clang-format on
