#ifndef MESSAGEREGISTRY_V1
#define MESSAGEREGISTRY_V1

#include "MessageRegistry_v1.h"
#include "Resource_v1.h"

enum class MessageRegistryV1ClearingType
{
    SameOriginOfCondition,
};
enum class MessageRegistryV1ParamType
{
    string,
    number,
};
struct MessageRegistryV1OemActions
{};
struct MessageRegistryV1Actions
{
    MessageRegistryV1OemActions oem;
};
struct MessageRegistryV1ClearingLogic
{
    MessageRegistryV1ClearingType clearsIf;
    std::string clearsMessage;
    bool clearsAll;
};
struct MessageRegistryV1Message
{
    std::string description;
    std::string message;
    std::string severity;
    int64_t numberOfArgs;
    MessageRegistryV1ParamType paramTypes;
    std::string resolution;
    ResourceV1Resource oem;
    MessageRegistryV1ClearingLogic clearingLogic;
    std::string longDescription;
    std::string argDescriptions;
    std::string argLongDescriptions;
    ResourceV1Resource messageSeverity;
};
struct MessageRegistryV1MessageProperty
{};
struct MessageRegistryV1MessageRegistry
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string language;
    std::string registryPrefix;
    std::string registryVersion;
    std::string owningEntity;
    MessageRegistryV1MessageProperty messages;
    MessageRegistryV1Actions actions;
};
#endif
