#ifndef JOB_V1
#define JOB_V1

#include "Job_v1.h"
#include "Message_v1.h"
#include "Resource_v1.h"
#include "Schedule_v1.h"

#include <chrono>

enum class Job_v1_JobState
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
struct Job_v1_Actions
{
    Job_v1_OemActions oem;
};
struct Job_v1_Job
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource jobStatus;
    Job_v1_JobState jobState;
    std::chrono::time_point startTime;
    std::chrono::time_point endTime;
    std::string maxExecutionTime;
    int64_t percentComplete;
    std::string createdBy;
    Schedule_v1_Schedule schedule;
    bool hidePayload;
    Job_v1_Payload payload;
    std::string stepOrder;
    Message_v1_Message messages;
    Job_v1_Actions actions;
};
struct Job_v1_OemActions
{};
struct Job_v1_Payload
{
    std::string targetUri;
    std::string httpOperation;
    std::string httpHeaders;
    std::string jsonBody;
};
#endif
