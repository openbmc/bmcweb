#ifndef ACTIONINFO_V1
#define ACTIONINFO_V1

#include "ActionInfo_v1.h"
#include "Resource_v1.h"

enum class ActionInfo_v1_ParameterTypes {
    Boolean,
    Number,
    NumberArray,
    String,
    StringArray,
    Object,
    ObjectArray,
};
struct ActionInfo_v1_ActionInfo
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ActionInfo_v1_Parameters parameters;
};
struct ActionInfo_v1_Parameters
{
    std::string name;
    bool required;
    ActionInfo_v1_ParameterTypes dataType;
    std::string objectDataType;
    std::string allowableValues;
    double minimumValue;
    double maximumValue;
};
#endif
