#ifndef ACTIONINFO_V1
#define ACTIONINFO_V1

#include "ActionInfo_v1.h"
#include "Resource_v1.h"

enum class ActionInfoV1ParameterTypes
{
    Boolean,
    Number,
    NumberArray,
    String,
    StringArray,
    Object,
    ObjectArray,
};
struct ActionInfoV1Parameters
{
    std::string name;
    bool required;
    ActionInfoV1ParameterTypes dataType;
    std::string objectDataType;
    std::string allowableValues;
    double minimumValue;
    double maximumValue;
};
struct ActionInfoV1ActionInfo
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ActionInfoV1Parameters parameters;
};
#endif
