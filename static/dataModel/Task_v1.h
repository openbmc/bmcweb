#ifndef TASK_V1
#define TASK_V1

#include "Message_v1.h"
#include "Resource_v1.h"
#include "Task_v1.h"

#include <chrono>

enum class TaskV1TaskState
{
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
struct TaskV1OemActions
{};
struct TaskV1Actions
{
    TaskV1OemActions oem;
};
struct TaskV1Payload
{
    std::string targetUri;
    std::string httpOperation;
    std::string httpHeaders;
    std::string jsonBody;
};
struct TaskV1Task
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    TaskV1TaskState taskState;
    std::chrono::time_point<std::chrono::system_clock> startTime;
    std::chrono::time_point<std::chrono::system_clock> endTime;
    ResourceV1Resource taskStatus;
    MessageV1Message messages;
    TaskV1Actions actions;
    std::string taskMonitor;
    TaskV1Payload payload;
    bool hidePayload;
    int64_t percentComplete;
};
#endif
