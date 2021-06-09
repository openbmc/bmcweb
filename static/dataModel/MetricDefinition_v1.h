#ifndef METRICDEFINITION_V1
#define METRICDEFINITION_V1

#include "MetricDefinition_v1.h"
#include "PhysicalContext_v1.h"
#include "Resource_v1.h"

#include <chrono>

enum class MetricDefinitionV1Calculable
{
    NonCalculatable,
    Summable,
    NonSummable,
};
enum class MetricDefinitionV1CalculationAlgorithmEnum
{
    Average,
    Maximum,
    Minimum,
    OEM,
};
enum class MetricDefinitionV1ImplementationType
{
    PhysicalSensor,
    Calculated,
    Synthesized,
    DigitalMeter,
};
enum class MetricDefinitionV1MetricDataType
{
    Boolean,
    DateTime,
    Decimal,
    Integer,
    String,
    Enumeration,
};
enum class MetricDefinitionV1MetricType
{
    Numeric,
    Discrete,
    Gauge,
    Counter,
    Countdown,
    String,
};
struct MetricDefinitionV1OemActions
{};
struct MetricDefinitionV1Actions
{
    MetricDefinitionV1OemActions oem;
};
struct MetricDefinitionV1CalculationParamsType
{
    std::string sourceMetric;
    std::string resultMetric;
};
struct MetricDefinitionV1Wildcard
{
    std::string name;
    std::string values;
};
struct MetricDefinitionV1MetricDefinition
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    MetricDefinitionV1MetricType metricType;
    MetricDefinitionV1MetricDataType metricDataType;
    std::string units;
    MetricDefinitionV1ImplementationType implementation;
    MetricDefinitionV1Calculable calculable;
    bool isLinear;
    MetricDefinitionV1Wildcard wildcards;
    std::string metricProperties;
    MetricDefinitionV1CalculationParamsType calculationParameters;
    PhysicalContextV1PhysicalContext physicalContext;
    std::chrono::milliseconds sensingInterval;
    std::string discreteValues;
    int64_t precision;
    double accuracy;
    double calibration;
    std::chrono::milliseconds timestampAccuracy;
    double minReadingRange;
    double maxReadingRange;
    MetricDefinitionV1CalculationAlgorithmEnum calculationAlgorithm;
    std::chrono::milliseconds calculationTimeInterval;
    MetricDefinitionV1Actions actions;
    std::string oEMCalculationAlgorithm;
};
#endif
