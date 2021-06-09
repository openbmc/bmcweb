#ifndef SESSIONSERVICE_V1
#define SESSIONSERVICE_V1

#include "Resource_v1.h"
#include "SessionCollection_v1.h"
#include "SessionService_v1.h"

struct SessionServiceV1OemActions
{};
struct SessionServiceV1Actions
{
    SessionServiceV1OemActions oem;
};
struct SessionServiceV1SessionService
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    bool serviceEnabled;
    int64_t sessionTimeout;
    SessionCollectionV1SessionCollection sessions;
    SessionServiceV1Actions actions;
};
#endif
