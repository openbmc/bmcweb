#ifndef MESSAGEREGISTRY_V1
#define MESSAGEREGISTRY_V1

#include "MessageRegistry_v1.h"
#include "Resource_v1.h"

enum class MessageRegistry_v1_ClearingType {
    SameOriginOfCondition,
};
enum class MessageRegistry_v1_ParamType {
    string,
    number,
};
struct MessageRegistry_v1_Actions
{
    MessageRegistry_v1_OemActions oem;
};
struct MessageRegistry_v1_ClearingLogic
{
    MessageRegistry_v1_ClearingType clearsIf;
    std::string clearsMessage;
    bool clearsAll;
};
struct MessageRegistry_v1_Message
{
    std::string description;
    std::string message;
    std::string severity;
    int64_t numberOfArgs;
    MessageRegistry_v1_ParamType paramTypes;
    std::string resolution;
    Resource_v1_Resource oem;
    MessageRegistry_v1_ClearingLogic clearingLogic;
    std::string longDescription;
    std::string argDescriptions;
    std::string argLongDescriptions;
    Resource_v1_Resource messageSeverity;
};
struct MessageRegistry_v1_MessageProperty
{
};
struct MessageRegistry_v1_MessageRegistry
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string language;
    std::string registryPrefix;
    std::string registryVersion;
    std::string owningEntity;
    MessageRegistry_v1_MessageProperty messages;
    MessageRegistry_v1_Actions actions;
};
struct MessageRegistry_v1_OemActions
{
};
#endif
