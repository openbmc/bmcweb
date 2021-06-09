#ifndef SESSIONSERVICE_V1
#define SESSIONSERVICE_V1

#include "Resource_v1.h"
#include "SessionCollection_v1.h"
#include "SessionService_v1.h"

struct SessionService_v1_Actions
{
    SessionService_v1_OemActions oem;
};
struct SessionService_v1_OemActions
{};
struct SessionService_v1_SessionService
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource status;
    bool serviceEnabled;
    int64_t sessionTimeout;
    SessionCollection_v1_SessionCollection sessions;
    SessionService_v1_Actions actions;
};
#endif
