#ifndef JOBSERVICE_V1
#define JOBSERVICE_V1

#include "JobCollection_v1.h"
#include "JobService_v1.h"
#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

#include <chrono>

struct JobServiceV1OemActions
{};
struct JobServiceV1Actions
{
    JobServiceV1OemActions oem;
};
struct JobServiceV1JobServiceCapabilities
{
    int64_t maxJobs;
    int64_t maxSteps;
    bool scheduling;
};
struct JobServiceV1JobService
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::chrono::time_point<std::chrono::system_clock> dateTime;
    bool serviceEnabled;
    JobServiceV1JobServiceCapabilities serviceCapabilities;
    ResourceV1Resource status;
    NavigationReferenceRedfish log;
    JobCollectionV1JobCollection jobs;
    JobServiceV1Actions actions;
};
#endif
