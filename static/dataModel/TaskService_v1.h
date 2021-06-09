#ifndef TASKSERVICE_V1
#define TASKSERVICE_V1

#include <chrono>
#include "Resource_v1.h"
#include "TaskCollection_v1.h"
#include "TaskService_v1.h"

enum class TaskService_v1_OverWritePolicy {
    Manual,
    Oldest,
};
struct TaskService_v1_Actions
{
    TaskService_v1_OemActions oem;
};
struct TaskService_v1_OemActions
{
};
struct TaskService_v1_TaskService
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    TaskService_v1_OverWritePolicy completedTaskOverWritePolicy;
    std::chrono::time_point dateTime;
    bool lifeCycleEventOnTaskStateChange;
    bool serviceEnabled;
    Resource_v1_Resource status;
    TaskCollection_v1_TaskCollection tasks;
    TaskService_v1_Actions actions;
};
#endif
