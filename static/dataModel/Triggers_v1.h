#ifndef TRIGGERS_V1
#define TRIGGERS_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"
#include "Triggers_v1.h"

#include <chrono>

enum class Triggers_v1_DirectionOfCrossingEnum
{
    Increasing,
    Decreasing,
};
enum class Triggers_v1_DiscreteTriggerConditionEnum
{
    Specified,
    Changed,
};
enum class Triggers_v1_MetricTypeEnum
{
    Numeric,
    Discrete,
};
enum class Triggers_v1_ThresholdActivation
{
    Increasing,
    Decreasing,
    Either,
};
enum class Triggers_v1_TriggerActionEnum
{
    LogToLogService,
    RedfishEvent,
    RedfishMetricReport,
};
struct Triggers_v1_Actions
{
    Triggers_v1_OemActions oem;
};
struct Triggers_v1_DiscreteTrigger
{
    std::string name;
    std::string value;
    std::chrono::milliseconds dwellTime;
    Resource_v1_Resource severity;
};
struct Triggers_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ metricReportDefinitions;
};
struct Triggers_v1_OemActions
{};
struct Triggers_v1_Threshold
{
    double reading;
    Triggers_v1_ThresholdActivation activation;
    std::chrono::milliseconds dwellTime;
};
struct Triggers_v1_Thresholds
{
    Triggers_v1_Threshold upperWarning;
    Triggers_v1_Threshold upperCritical;
    Triggers_v1_Threshold lowerWarning;
    Triggers_v1_Threshold lowerCritical;
};
struct Triggers_v1_Triggers
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Triggers_v1_MetricTypeEnum metricType;
    Triggers_v1_TriggerActionEnum triggerActions;
    Triggers_v1_Thresholds numericThresholds;
    Triggers_v1_DiscreteTriggerConditionEnum discreteTriggerCondition;
    Triggers_v1_DiscreteTrigger discreteTriggers;
    Resource_v1_Resource status;
    Triggers_v1_Wildcard wildcards;
    std::string metricProperties;
    Triggers_v1_Actions actions;
    std::string eventTriggers;
    Triggers_v1_Links links;
};
struct Triggers_v1_Wildcard
{
    std::string name;
    std::string values;
};
#endif
