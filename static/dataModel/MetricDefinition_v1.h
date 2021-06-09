#ifndef METRICDEFINITION_V1
#define METRICDEFINITION_V1

#include "MetricDefinition_v1.h"
#include "PhysicalContext_v1.h"
#include "Resource_v1.h"

#include <chrono>

enum class MetricDefinition_v1_Calculable
{
    NonCalculatable,
    Summable,
    NonSummable,
};
enum class MetricDefinition_v1_CalculationAlgorithmEnum
{
    Average,
    Maximum,
    Minimum,
    OEM,
};
enum class MetricDefinition_v1_ImplementationType
{
    PhysicalSensor,
    Calculated,
    Synthesized,
    DigitalMeter,
};
enum class MetricDefinition_v1_MetricDataType
{
    Boolean,
    DateTime,
    Decimal,
    Integer,
    String,
    Enumeration,
};
enum class MetricDefinition_v1_MetricType
{
    Numeric,
    Discrete,
    Gauge,
    Counter,
    Countdown,
};
struct MetricDefinition_v1_Actions
{
    MetricDefinition_v1_OemActions oem;
};
struct MetricDefinition_v1_CalculationParamsType
{
    std::string sourceMetric;
    std::string resultMetric;
};
struct MetricDefinition_v1_MetricDefinition
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    MetricDefinition_v1_MetricType metricType;
    MetricDefinition_v1_MetricDataType metricDataType;
    std::string units;
    MetricDefinition_v1_ImplementationType implementation;
    MetricDefinition_v1_Calculable calculable;
    bool isLinear;
    MetricDefinition_v1_Wildcard wildcards;
    std::string metricProperties;
    MetricDefinition_v1_CalculationParamsType calculationParameters;
    PhysicalContext_v1_PhysicalContext physicalContext;
    std::chrono::milliseconds sensingInterval;
    std::string discreteValues;
    int64_t precision;
    double accuracy;
    double calibration;
    std::chrono::milliseconds timestampAccuracy;
    double minReadingRange;
    double maxReadingRange;
    MetricDefinition_v1_CalculationAlgorithmEnum calculationAlgorithm;
    std::chrono::milliseconds calculationTimeInterval;
    MetricDefinition_v1_Actions actions;
    std::string oEMCalculationAlgorithm;
};
struct MetricDefinition_v1_OemActions
{};
struct MetricDefinition_v1_Wildcard
{
    std::string name;
    std::string values;
};
#endif
