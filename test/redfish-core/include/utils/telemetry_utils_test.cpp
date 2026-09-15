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

TEST(GetDbusReportPath, EscapesIdWithNonAlphanumericCharacter)
{
    EXPECT_EQ(getDbusReportPath("Test Report"),
              "/xyz/openbmc_project/Telemetry/Reports/TelemetryService/"
              "_54est_20Report");
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

TEST(GetDbusTriggerPath, EscapesIdWithNonAlphanumericCharacter)
{
    EXPECT_EQ(getDbusTriggerPath("Test Trigger"),
              "/xyz/openbmc_project/Telemetry/Triggers/TelemetryService/"
              "_54est_20Trigger");
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
                  "_54est_20Trigger"),
              std::optional<std::string>("Test Trigger"));
}

TEST(GetTriggerIdFromDbusPath, RoundTripsGetDbusTriggerPath)
{
    EXPECT_EQ(getTriggerIdFromDbusPath(getDbusTriggerPath("TestTrigger")),
              std::optional<std::string>("TestTrigger"));
    EXPECT_EQ(getTriggerIdFromDbusPath(getDbusTriggerPath("Test Trigger")),
              std::optional<std::string>("Test Trigger"));
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

TEST(GetChassisSensorNode, MatchesChassisNodeUri)
{
    const std::vector<std::string> uris{"/redfish/v1/Chassis/chassis1/Power"};
    ChassisSensorNodes matched;

    const ChassisSensorNodes expected{{"chassis1", "Power"}};

    EXPECT_EQ(getChassisSensorNode(uris, matched), std::nullopt);
    EXPECT_EQ(matched, expected);
}

TEST(GetChassisSensorNode, MatchesSensorUriAndDropsSensorId)
{
    const std::vector<std::string> uris{
        "/redfish/v1/Chassis/chassis1/Sensors/sensor1"};
    ChassisSensorNodes matched;

    const ChassisSensorNodes expected{{"chassis1", "Sensors"}};

    EXPECT_EQ(getChassisSensorNode(uris, matched), std::nullopt);
    EXPECT_EQ(matched, expected);
}

TEST(GetChassisSensorNode, ToleratesTrailingSlash)
{
    const std::vector<std::string> uris{
        "/redfish/v1/Chassis/chassis1/Power/",
        "/redfish/v1/Chassis/chassis2/Sensors/sensor1/",
        "/redfish/v1/Chassis/chassis3/Sensors/"};
    ChassisSensorNodes matched;

    const ChassisSensorNodes expected{{"chassis1", "Power"},
                                      {"chassis2", "Sensors"},
                                      {"chassis3", "Sensors"}};

    EXPECT_EQ(getChassisSensorNode(uris, matched), std::nullopt);
    EXPECT_EQ(matched, expected);
}

TEST(GetChassisSensorNode, DeduplicatesRepeatedUri)
{
    const std::vector<std::string> uris{"/redfish/v1/Chassis/chassis1/Power",
                                        "/redfish/v1/Chassis/chassis1/Power"};
    ChassisSensorNodes matched;

    const ChassisSensorNodes expected{{"chassis1", "Power"}};

    EXPECT_EQ(getChassisSensorNode(uris, matched), std::nullopt);
    EXPECT_EQ(matched.size(), size_t{1});
    EXPECT_EQ(matched, expected);
}

TEST(GetChassisSensorNode, MatchesBothUriFormsInOneCall)
{
    const std::vector<std::string> uris{
        "/redfish/v1/Chassis/chassis1/Power",
        "/redfish/v1/Chassis/chassis2/Sensors/sensor1"};
    ChassisSensorNodes matched;

    const ChassisSensorNodes expected{{"chassis1", "Power"},
                                      {"chassis2", "Sensors"}};

    EXPECT_EQ(getChassisSensorNode(uris, matched), std::nullopt);
    EXPECT_EQ(matched, expected);
}

TEST(GetChassisSensorNode, DecodesPercentEncodedChassisName)
{
    const std::vector<std::string> uris{
        "/redfish/v1/Chassis/chassis%201/Power"};
    ChassisSensorNodes matched;

    const ChassisSensorNodes expected{{"chassis 1", "Power"}};

    EXPECT_EQ(getChassisSensorNode(uris, matched), std::nullopt);
    EXPECT_EQ(matched, expected);
}

TEST(GetChassisSensorNode, AcceptsEmptyChassisSegment)
{
    const std::vector<std::string> uris{"/redfish/v1/Chassis//Power"};
    ChassisSensorNodes matched;

    const ChassisSensorNodes expected{{"", "Power"}};

    EXPECT_EQ(getChassisSensorNode(uris, matched), std::nullopt);
    EXPECT_EQ(matched, expected);
}

TEST(GetChassisSensorNode, AcceptsFragmentAndQuery)
{
    const std::vector<std::string> uris{
        "/redfish/v1/Chassis/chassis1#/Power",
        "/redfish/v1/Chassis/chassis2/Power?a=b"};
    ChassisSensorNodes matched;

    const ChassisSensorNodes expected{{"chassis1", "Power"},
                                      {"chassis2", "Power"}};

    EXPECT_EQ(getChassisSensorNode(uris, matched), std::nullopt);
    EXPECT_EQ(matched, expected);
}

TEST(GetChassisSensorNode, AppendsToPreexistingMatches)
{
    const std::vector<std::string> uris{"/redfish/v1/Chassis/chassis1/Power"};
    ChassisSensorNodes matched{{"pre", "Existing"}};

    const ChassisSensorNodes expected{{"chassis1", "Power"},
                                      {"pre", "Existing"}};

    EXPECT_EQ(getChassisSensorNode(uris, matched), std::nullopt);
    EXPECT_EQ(matched, expected);
}

TEST(GetChassisSensorNode, RejectsUriMatchingNeitherForm)
{
    const std::vector<std::string> badUris{
        "/redfish/v1/Chassis/chassis1", "/redfish/v1/Chassis",
        "/redfish/v1/Systems/system/Memory",
        "/redfish/v1/Chassis/chassis1/Sensors/sensor1/Extra",
        "/redfish/v1/CHASSIS/c1/Power"};

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
        "redfish/v1/Chassis/chassis1/Power",
        "http://host/redfish/v1/Chassis/chassis1/Power", ""};

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
    const std::vector<std::string> uris{"bad uri"};
    ChassisSensorNodes matched;

    std::optional<IncorrectMetricUri> result =
        getChassisSensorNode(uris, matched);
    const IncorrectMetricUri bad =
        result.value_or(IncorrectMetricUri{"<unset>", 999});
    EXPECT_EQ(bad.uri, "bad uri");
    EXPECT_EQ(bad.index, size_t{0});
    EXPECT_TRUE(matched.empty());
}

TEST(GetChassisSensorNode, ReportsIndexOfFailureAndKeepsEarlierMatches)
{
    const std::vector<std::string> uris{
        "/redfish/v1/Chassis/chassis1/Power",
        "/redfish/v1/Chassis/chassis2/Sensors/s1",
        "/redfish/v1/Systems/system/Memory",
        "/redfish/v1/Chassis/chassis9/Power"};
    ChassisSensorNodes matched;

    const ChassisSensorNodes expected{{"chassis1", "Power"},
                                      {"chassis2", "Sensors"}};

    std::optional<IncorrectMetricUri> result =
        getChassisSensorNode(uris, matched);
    const IncorrectMetricUri bad =
        result.value_or(IncorrectMetricUri{"<unset>", 999});
    EXPECT_EQ(bad.uri, "/redfish/v1/Systems/system/Memory");
    EXPECT_EQ(bad.index, size_t{2});
    EXPECT_EQ(matched, expected);
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

TEST(ToRedfishCollectionFunctions, KeepsInputOrderAndDuplicates)
{
    const std::vector<std::string> dbusEnums{
        "xyz.openbmc_project.Telemetry.Report.OperationType.Maximum",
        "xyz.openbmc_project.Telemetry.Report.OperationType.Minimum",
        "xyz.openbmc_project.Telemetry.Report.OperationType.Average",
        "xyz.openbmc_project.Telemetry.Report.OperationType.Summation",
        "xyz.openbmc_project.Telemetry.Report.OperationType.Maximum"};

    std::optional<nlohmann::json::array_t> result =
        toRedfishCollectionFunctions(dbusEnums);
    const std::optional<nlohmann::json::array_t> expected =
        nlohmann::json::array_t{"Maximum", "Minimum", "Average", "Summation",
                                "Maximum"};
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
