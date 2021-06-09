#ifndef JOBSERVICE_V1
#define JOBSERVICE_V1

#include "JobCollection_v1.h"
#include "JobService_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"

#include <chrono>

struct JobService_v1_Actions
{
    JobService_v1_OemActions oem;
};
struct JobService_v1_JobService
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::chrono::time_point dateTime;
    bool serviceEnabled;
    JobService_v1_JobServiceCapabilities serviceCapabilities;
    Resource_v1_Resource status;
    NavigationReference__ log;
    JobCollection_v1_JobCollection jobs;
    JobService_v1_Actions actions;
};
struct JobService_v1_JobServiceCapabilities
{
    int64_t maxJobs;
    int64_t maxSteps;
    bool scheduling;
};
struct JobService_v1_OemActions
{};
#endif
