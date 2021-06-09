#ifndef TASK_V1
#define TASK_V1

#include <chrono>
#include "Message_v1.h"
#include "Resource_v1.h"
#include "Task_v1.h"

enum class Task_v1_TaskState {
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
struct Task_v1_Actions
{
    Task_v1_OemActions oem;
};
struct Task_v1_OemActions
{
};
struct Task_v1_Payload
{
    std::string targetUri;
    std::string httpOperation;
    std::string httpHeaders;
    std::string jsonBody;
};
struct Task_v1_Task
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Task_v1_TaskState taskState;
    std::chrono::time_point startTime;
    std::chrono::time_point endTime;
    Resource_v1_Resource taskStatus;
    Message_v1_Message messages;
    Task_v1_Actions actions;
    std::string taskMonitor;
    Task_v1_Payload payload;
    bool hidePayload;
    int64_t percentComplete;
};
#endif
