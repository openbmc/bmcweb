#ifndef JOB_V1
#define JOB_V1

#include "Job_v1.h"
#include "Message_v1.h"
#include "Resource_v1.h"
#include "Schedule_v1.h"

#include <chrono>

enum class JobV1JobState
{
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
struct JobV1OemActions
{};
struct JobV1Actions
{
    JobV1OemActions oem;
};
struct JobV1Payload
{
    std::string targetUri;
    std::string httpOperation;
    std::string httpHeaders;
    std::string jsonBody;
};
struct JobV1Job
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource jobStatus;
    JobV1JobState jobState;
    std::chrono::time_point<std::chrono::system_clock> startTime;
    std::chrono::time_point<std::chrono::system_clock> endTime;
    std::string maxExecutionTime;
    int64_t percentComplete;
    std::string createdBy;
    ScheduleV1Schedule schedule;
    bool hidePayload;
    JobV1Payload payload;
    std::string stepOrder;
    MessageV1Message messages;
    JobV1Actions actions;
};
#endif
