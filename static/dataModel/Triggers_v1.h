#ifndef TRIGGERS_V1
#define TRIGGERS_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"
#include "Triggers_v1.h"

#include <chrono>

enum class TriggersV1DirectionOfCrossingEnum
{
    Increasing,
    Decreasing,
};
enum class TriggersV1DiscreteTriggerConditionEnum
{
    Specified,
    Changed,
};
enum class TriggersV1MetricTypeEnum
{
    Numeric,
    Discrete,
};
enum class TriggersV1ThresholdActivation
{
    Increasing,
    Decreasing,
    Either,
};
enum class TriggersV1TriggerActionEnum
{
    LogToLogService,
    RedfishEvent,
    RedfishMetricReport,
};
struct TriggersV1OemActions
{};
struct TriggersV1Actions
{
    TriggersV1OemActions oem;
};
struct TriggersV1DiscreteTrigger
{
    std::string name;
    std::string value;
    std::chrono::milliseconds dwellTime;
    ResourceV1Resource severity;
};
struct TriggersV1Links
{
    ResourceV1Resource oem;
    NavigationReferenceRedfish metricReportDefinitions;
};
struct TriggersV1Threshold
{
    double reading;
    TriggersV1ThresholdActivation activation;
    std::chrono::milliseconds dwellTime;
};
struct TriggersV1Thresholds
{
    TriggersV1Threshold upperWarning;
    TriggersV1Threshold upperCritical;
    TriggersV1Threshold lowerWarning;
    TriggersV1Threshold lowerCritical;
};
struct TriggersV1Wildcard
{
    std::string name;
    std::string values;
};
struct TriggersV1Triggers
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    TriggersV1MetricTypeEnum metricType;
    TriggersV1TriggerActionEnum triggerActions;
    TriggersV1Thresholds numericThresholds;
    TriggersV1DiscreteTriggerConditionEnum discreteTriggerCondition;
    TriggersV1DiscreteTrigger discreteTriggers;
    ResourceV1Resource status;
    TriggersV1Wildcard wildcards;
    std::string metricProperties;
    TriggersV1Actions actions;
    std::string eventTriggers;
    TriggersV1Links links;
};
#endif
