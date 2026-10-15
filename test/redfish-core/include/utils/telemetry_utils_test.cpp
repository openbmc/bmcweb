// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "generated/enums/metric_report_definition.hpp"
#include "utils/telemetry_utils.hpp"

#include <boost/container/flat_set.hpp>
#include <nlohmann/json.hpp>

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace redfish::telemetry
{
namespace
{

using ChassisSensorNodes =
    boost::container::flat_set<std::pair<std::string, std::string>>;

TEST(GetDbusReportPath, AppendsIdToReportsCollection)
{
    EXPECT_EQ(getDbusReportPath("TestReport"),
              "/xyz/openbmc_project/Telemetry/Reports/TelemetryService/"
              "TestReport");
    EXPECT_EQ(getDbusReportPath("Test_Report"),
              "/xyz/openbmc_project/Telemetry/Reports/TelemetryService/"
              "Test_Report");
    EXPECT_EQ(getDbusReportPath("1Report"),
              "/xyz/openbmc_project/Telemetry/Reports/TelemetryService/"
              "1Report");
}

TEST(GetDbusReportPath, EmptyIdThrows)
{
    EXPECT_THROW(getDbusReportPath(""), std::invalid_argument);
}

TEST(GetDbusReportPath, EscapesIdStartingWithUnderscore)
{
    EXPECT_EQ(getDbusReportPath("_Report"),
              "/xyz/openbmc_project/Telemetry/Reports/TelemetryService/"
              "_5fReport");
}

TEST(GetDbusTriggerPath, AppendsIdToTriggersCollection)
{
    EXPECT_EQ(getDbusTriggerPath("TestTrigger"),
              "/xyz/openbmc_project/Telemetry/Triggers/TelemetryService/"
              "TestTrigger");
}

TEST(GetDbusTriggerPath, EmptyIdThrows)
{
    EXPECT_THROW(getDbusTriggerPath(""), std::invalid_argument);
}

TEST(GetDbusTriggerPath, EscapesIdStartingWithUnderscore)
{
    EXPECT_EQ(getDbusTriggerPath("_Trigger"),
              "/xyz/openbmc_project/Telemetry/Triggers/TelemetryService/"
              "_5fTrigger");
}

TEST(GetTriggerIdFromDbusPath, ReturnsLastSegment)
{
    EXPECT_EQ(getTriggerIdFromDbusPath(
                  "/xyz/openbmc_project/Telemetry/Triggers/TelemetryService/"
                  "TestTrigger"),
              std::optional<std::string>("TestTrigger"));
    EXPECT_EQ(getTriggerIdFromDbusPath(
                  "/xyz/openbmc_project/Telemetry/Triggers/TelemetryService/"
                  "Test_Trigger"),
              std::optional<std::string>("Test_Trigger"));
}

TEST(GetTriggerIdFromDbusPath, DecodesEscapedSegment)
{
    EXPECT_EQ(getTriggerIdFromDbusPath(
                  "/xyz/openbmc_project/Telemetry/Triggers/TelemetryService/"
                  "_5fTrigger"),
              std::optional<std::string>("_Trigger"));
}

TEST(GetTriggerIdFromDbusPath, RoundTripsGetDbusTriggerPath)
{
    EXPECT_EQ(getTriggerIdFromDbusPath(getDbusTriggerPath("TestTrigger")),
              std::optional<std::string>("TestTrigger"));
    EXPECT_EQ(getTriggerIdFromDbusPath(getDbusTriggerPath("Test_Trigger")),
              std::optional<std::string>("Test_Trigger"));
    EXPECT_EQ(getTriggerIdFromDbusPath(getDbusTriggerPath("_Trigger")),
              std::optional<std::string>("_Trigger"));
    EXPECT_EQ(getTriggerIdFromDbusPath(getDbusTriggerPath("a.b")),
              std::optional<std::string>("a.b"));
}

TEST(GetTriggerIdFromDbusPath, RejectsEmptyId)
{
    EXPECT_EQ(getTriggerIdFromDbusPath(
                  "/xyz/openbmc_project/Telemetry/Triggers/TelemetryService/"),
              std::nullopt);
    EXPECT_EQ(getTriggerIdFromDbusPath(
                  "/xyz/openbmc_project/Telemetry/Triggers/TelemetryService/"
                  "_zz"),
              std::nullopt);
}

TEST(GetTriggerIdFromDbusPath, RejectsPathOutsideTriggersCollection)
{
    EXPECT_EQ(getTriggerIdFromDbusPath(
                  "/xyz/openbmc_project/Telemetry/Triggers/TelemetryService"),
              std::nullopt);
    EXPECT_EQ(getTriggerIdFromDbusPath(
                  "/xyz/openbmc_project/Telemetry/Reports/TelemetryService/"
                  "TestReport"),
              std::nullopt);
    EXPECT_EQ(getTriggerIdFromDbusPath(
                  "/xyz/openbmc_project/Telemetry/Triggers/TelemetryService/"
                  "a/b"),
              std::nullopt);
    EXPECT_EQ(getTriggerIdFromDbusPath(""), std::nullopt);
    EXPECT_EQ(getTriggerIdFromDbusPath("NotAPath"), std::nullopt);
}

TEST(GetChassisSensorNode, EmptyUriListMatchesNothing)
{
    const std::vector<std::string> uris;
    ChassisSensorNodes matched;

    EXPECT_EQ(getChassisSensorNode(uris, matched), std::nullopt);
    EXPECT_TRUE(matched.empty());
}

TEST(GetChassisSensorNode, RejectsEmptyChassisName)
{
    const std::vector<std::string> badUris{
        "/redfish/v1/Chassis//Sensors", "/redfish/v1/Chassis//Sensors/sensor1"};

    for (const std::string& badUri : badUris)
    {
        const std::vector<std::string> uris{badUri};
        ChassisSensorNodes matched;

        std::optional<IncorrectMetricUri> result =
            getChassisSensorNode(uris, matched);
        const IncorrectMetricUri bad =
            result.value_or(IncorrectMetricUri{"<unset>", 999});
        EXPECT_EQ(bad.uri, badUri);
        EXPECT_EQ(bad.index, size_t{0});
        EXPECT_TRUE(matched.empty());
    }
}

TEST(GetChassisSensorNode, RejectsUriMatchingNeitherForm)
{
    const std::vector<std::string> badUris{
        "/redfish/v1/Chassis/chassis1", "/redfish/v1/Chassis",
        "/redfish/v1/Systems/system/Memory",
        "/redfish/v1/Chassis/chassis1/Sensors/sensor1/Extra",
        "/redfish/v1/CHASSIS/chassis1/Sensors/sensor1"};

    for (const std::string& badUri : badUris)
    {
        const std::vector<std::string> uris{badUri};
        ChassisSensorNodes matched;

        std::optional<IncorrectMetricUri> result =
            getChassisSensorNode(uris, matched);
        const IncorrectMetricUri bad =
            result.value_or(IncorrectMetricUri{"<unset>", 999});
        EXPECT_EQ(bad.uri, badUri);
        EXPECT_EQ(bad.index, size_t{0});
        EXPECT_TRUE(matched.empty());
    }
}

TEST(GetChassisSensorNode, RejectsNonAbsoluteUri)
{
    const std::vector<std::string> badUris{
        "redfish/v1/Chassis/chassis1/Sensors/sensor1", ""};

    for (const std::string& badUri : badUris)
    {
        const std::vector<std::string> uris{badUri};
        ChassisSensorNodes matched;

        std::optional<IncorrectMetricUri> result =
            getChassisSensorNode(uris, matched);
        const IncorrectMetricUri bad =
            result.value_or(IncorrectMetricUri{"<unset>", 999});
        EXPECT_EQ(bad.uri, badUri);
        EXPECT_EQ(bad.index, size_t{0});
        EXPECT_TRUE(matched.empty());
    }
}

TEST(GetChassisSensorNode, RejectsUnparsableUri)
{
    const std::vector<std::string> badUris{
        "bad uri", "http://host/redfish/v1/Chassis/chassis1/Sensors/sensor1"};

    for (const std::string& badUri : badUris)
    {
        const std::vector<std::string> uris{badUri};
        ChassisSensorNodes matched;

        std::optional<IncorrectMetricUri> result =
            getChassisSensorNode(uris, matched);
        const IncorrectMetricUri bad =
            result.value_or(IncorrectMetricUri{"<unset>", 999});
        EXPECT_EQ(bad.uri, badUri);
        EXPECT_EQ(bad.index, size_t{0});
        EXPECT_TRUE(matched.empty());
    }
}

TEST(GetChassisSensorNode, ReportsIndexOfFirstIncorrectUri)
{
    const std::vector<std::string> uris{
        "/redfish/v1/Chassis/chassis1/Sensors/sensor1#/Reading",
        "/redfish/v1/Chassis/chassis2/Sensors/sensor2#/Reading",
        "/redfish/v1/Systems/system/Memory",
        "/redfish/v1/Chassis/chassis3/Sensors/sensor3#/Reading"};
    ChassisSensorNodes matched;

    std::optional<IncorrectMetricUri> result =
        getChassisSensorNode(uris, matched);
    const IncorrectMetricUri bad =
        result.value_or(IncorrectMetricUri{"<unset>", 999});
    EXPECT_EQ(bad.uri, "/redfish/v1/Systems/system/Memory");
    EXPECT_EQ(bad.index, size_t{2});
}

TEST(ToRedfishCollectionFunction, ConvertsKnownOperationType)
{
    EXPECT_EQ(toRedfishCollectionFunction(
                  "xyz.openbmc_project.Telemetry.Report.OperationType.Maximum"),
              metric_report_definition::CalculationAlgorithmEnum::Maximum);
    EXPECT_EQ(toRedfishCollectionFunction(
                  "xyz.openbmc_project.Telemetry.Report.OperationType.Minimum"),
              metric_report_definition::CalculationAlgorithmEnum::Minimum);
    EXPECT_EQ(toRedfishCollectionFunction(
                  "xyz.openbmc_project.Telemetry.Report.OperationType.Average"),
              metric_report_definition::CalculationAlgorithmEnum::Average);
    EXPECT_EQ(
        toRedfishCollectionFunction(
            "xyz.openbmc_project.Telemetry.Report.OperationType.Summation"),
        metric_report_definition::CalculationAlgorithmEnum::Summation);
}

TEST(ToRedfishCollectionFunction, UnknownOperationTypeIsInvalid)
{
    EXPECT_EQ(toRedfishCollectionFunction(""),
              metric_report_definition::CalculationAlgorithmEnum::Invalid);
    EXPECT_EQ(toRedfishCollectionFunction(
                  "xyz.openbmc_project.Telemetry.Report.OperationType.Invalid"),
              metric_report_definition::CalculationAlgorithmEnum::Invalid);
    EXPECT_EQ(toRedfishCollectionFunction("Maximum"),
              metric_report_definition::CalculationAlgorithmEnum::Invalid);
    EXPECT_EQ(toRedfishCollectionFunction(
                  "xyz.openbmc_project.Telemetry.Report.OperationType.maximum"),
              metric_report_definition::CalculationAlgorithmEnum::Invalid);
}

TEST(ToDbusCollectionFunction, ConvertsKnownCalculationAlgorithm)
{
    EXPECT_EQ(toDbusCollectionFunction("Maximum"),
              "xyz.openbmc_project.Telemetry.Report.OperationType.Maximum");
    EXPECT_EQ(toDbusCollectionFunction("Minimum"),
              "xyz.openbmc_project.Telemetry.Report.OperationType.Minimum");
    EXPECT_EQ(toDbusCollectionFunction("Average"),
              "xyz.openbmc_project.Telemetry.Report.OperationType.Average");
    EXPECT_EQ(toDbusCollectionFunction("Summation"),
              "xyz.openbmc_project.Telemetry.Report.OperationType.Summation");
}

TEST(ToDbusCollectionFunction, UnknownCalculationAlgorithmIsEmpty)
{
    EXPECT_EQ(toDbusCollectionFunction(""), "");
    EXPECT_EQ(toDbusCollectionFunction("Invalid"), "");
    EXPECT_EQ(toDbusCollectionFunction("maximum"), "");
    EXPECT_EQ(toDbusCollectionFunction(
                  "xyz.openbmc_project.Telemetry.Report.OperationType.Maximum"),
              "");
}

TEST(ToRedfishCollectionFunctions, EmptyInputGivesEmptyArray)
{
    const std::vector<std::string> dbusEnums;

    std::optional<nlohmann::json::array_t> result =
        toRedfishCollectionFunctions(dbusEnums);
    const std::optional<nlohmann::json::array_t> expected =
        nlohmann::json::array_t{};
    EXPECT_EQ(result, expected);
}

TEST(ToRedfishCollectionFunctions, ConvertsEachKnownOperationType)
{
    const std::vector<std::string> dbusEnums{
        "xyz.openbmc_project.Telemetry.Report.OperationType.Maximum",
        "xyz.openbmc_project.Telemetry.Report.OperationType.Minimum",
        "xyz.openbmc_project.Telemetry.Report.OperationType.Average",
        "xyz.openbmc_project.Telemetry.Report.OperationType.Summation"};

    std::optional<nlohmann::json::array_t> result =
        toRedfishCollectionFunctions(dbusEnums);
    const std::optional<nlohmann::json::array_t> expected =
        nlohmann::json::array_t{"Maximum", "Minimum", "Average", "Summation"};
    EXPECT_EQ(result, expected);
}

TEST(ToRedfishCollectionFunctions, SingleUnknownValueDiscardsWholeArray)
{
    const std::vector<std::string> withBogus{
        "xyz.openbmc_project.Telemetry.Report.OperationType.Maximum", "bogus"};
    const std::vector<std::string> redfishSpelling{"Maximum"};
    const std::vector<std::string> emptyString{""};

    EXPECT_EQ(toRedfishCollectionFunctions(withBogus), std::nullopt);
    EXPECT_EQ(toRedfishCollectionFunctions(redfishSpelling), std::nullopt);
    EXPECT_EQ(toRedfishCollectionFunctions(emptyString), std::nullopt);
}

} // namespace
} // namespace redfish::telemetry
