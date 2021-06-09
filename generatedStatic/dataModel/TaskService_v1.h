#ifndef TASKSERVICE_V1
#define TASKSERVICE_V1

#include "Resource_v1.h"
#include "TaskCollection_v1.h"
#include "TaskService_v1.h"

#include <chrono>

enum class TaskServiceV1OverWritePolicy
{
    Manual,
    Oldest,
};
struct TaskServiceV1OemActions
{};
struct TaskServiceV1Actions
{
    TaskServiceV1OemActions oem;
};
struct TaskServiceV1TaskService
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    TaskServiceV1OverWritePolicy completedTaskOverWritePolicy;
    std::chrono::time_point<std::chrono::system_clock> dateTime;
    bool lifeCycleEventOnTaskStateChange;
    bool serviceEnabled;
    ResourceV1Resource status;
    TaskCollectionV1TaskCollection tasks;
    TaskServiceV1Actions actions;
    int64_t taskAutoDeleteTimeoutMinutes;
};
#endif
