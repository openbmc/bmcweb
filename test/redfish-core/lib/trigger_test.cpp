// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "generated/enums/resource.hpp"
#include "generated/enums/triggers.hpp"
#include "trigger.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(GetTriggerActions, MapsDbusActions)
{
    const std::vector<std::string> dbusActions{
        "xyz.openbmc_project.Telemetry.Trigger.TriggerAction.UpdateReport",
        "xyz.openbmc_project.Telemetry.Trigger.TriggerAction.LogToRedfishEventLog"};

    const std::optional<nlohmann::json::array_t> expected =
        nlohmann::json::array_t{"RedfishMetricReport", "RedfishEvent"};
    EXPECT_EQ(telemetry::getTriggerActions(dbusActions), expected);
}

TEST(GetTriggerActions, UnmappedActionDiscardsAll)
{
    const std::vector<std::string> dbusActions{
        "xyz.openbmc_project.Telemetry.Trigger.TriggerAction.UpdateReport",
        "xyz.openbmc_project.Telemetry.Trigger.TriggerAction.LogToJournal"};

    EXPECT_EQ(telemetry::getTriggerActions(dbusActions), std::nullopt);
}

TEST(GetDiscreteTriggers, MapsDbusThresholds)
{
    const std::vector<telemetry::DiscreteThresholdParams> dbusThresholds{
        {"Normal", "xyz.openbmc_project.Telemetry.Trigger.Severity.OK", 1000,
         "0"},
        {"High", "xyz.openbmc_project.Telemetry.Trigger.Severity.Warning", 250,
         "80"},
        {"Fault", "xyz.openbmc_project.Telemetry.Trigger.Severity.Critical",
         5000, "95"}};

    const std::optional<nlohmann::json::array_t> expected = R"([
        {"Name": "Normal", "Severity": "OK", "DwellTime": "PT1.000S",
         "Value": "0"},
        {"Name": "High", "Severity": "Warning", "DwellTime": "PT0.250S",
         "Value": "80"},
        {"Name": "Fault", "Severity": "Critical", "DwellTime": "PT5.000S",
         "Value": "95"}
    ])"_json.get<nlohmann::json::array_t>();
    EXPECT_EQ(telemetry::getDiscreteTriggers(dbusThresholds), expected);
}

TEST(GetDiscreteTriggers, DwellTimeOutOfRangeGivesNullopt)
{
    const std::vector<telemetry::DiscreteThresholdParams> dbusThresholds{
        {"High", "xyz.openbmc_project.Telemetry.Trigger.Severity.Warning",
         std::numeric_limits<uint64_t>::max(), "80"}};

    EXPECT_EQ(telemetry::getDiscreteTriggers(dbusThresholds), std::nullopt);
}

TEST(GetNumericThresholds, MapsDbusThresholds)
{
    const std::vector<telemetry::NumericThresholdParams> dbusThresholds{
        {"xyz.openbmc_project.Telemetry.Trigger.Type.UpperCritical", 1000,
         "xyz.openbmc_project.Telemetry.Trigger.Direction.Increasing", 95.0},
        {"xyz.openbmc_project.Telemetry.Trigger.Type.UpperWarning", 250,
         "xyz.openbmc_project.Telemetry.Trigger.Direction.Increasing", 85.5},
        {"xyz.openbmc_project.Telemetry.Trigger.Type.LowerWarning", 250,
         "xyz.openbmc_project.Telemetry.Trigger.Direction.Decreasing", 10.5},
        {"xyz.openbmc_project.Telemetry.Trigger.Type.LowerCritical", 5000,
         "xyz.openbmc_project.Telemetry.Trigger.Direction.Either", 5.0}};

    const std::optional<nlohmann::json::object_t> expected = R"({
        "UpperCritical": {"Reading": 95.0, "Activation": "Increasing",
                          "DwellTime": "PT1.000S"},
        "UpperWarning": {"Reading": 85.5, "Activation": "Increasing",
                         "DwellTime": "PT0.250S"},
        "LowerWarning": {"Reading": 10.5, "Activation": "Decreasing",
                         "DwellTime": "PT0.250S"},
        "LowerCritical": {"Reading": 5.0, "Activation": "Either",
                          "DwellTime": "PT5.000S"}
    })"_json.get<nlohmann::json::object_t>();
    EXPECT_EQ(telemetry::getNumericThresholds(dbusThresholds), expected);
}

TEST(GetNumericThresholds, DwellTimeOutOfRangeGivesNullopt)
{
    const std::vector<telemetry::NumericThresholdParams> dbusThresholds{
        {"xyz.openbmc_project.Telemetry.Trigger.Type.UpperCritical",
         std::numeric_limits<uint64_t>::max(),
         "xyz.openbmc_project.Telemetry.Trigger.Direction.Increasing", 95.0}};

    EXPECT_EQ(telemetry::getNumericThresholds(dbusThresholds), std::nullopt);
}

TEST(TriggerEnumConversion, ToRedfishTriggerActionRejectsUnmappedDbusActions)
{
    EXPECT_EQ(
        telemetry::toRedfishTriggerAction(
            "xyz.openbmc_project.Telemetry.Trigger.TriggerAction.LogToJournal"),
        triggers::TriggerActionEnum::Invalid);
    EXPECT_EQ(telemetry::toRedfishTriggerAction(""),
              triggers::TriggerActionEnum::Invalid);
}

TEST(TriggerEnumConversion, ToDbusTriggerActionMapsRedfishActions)
{
    EXPECT_EQ(
        telemetry::toDbusTriggerAction("RedfishMetricReport"),
        "xyz.openbmc_project.Telemetry.Trigger.TriggerAction.UpdateReport");
    EXPECT_EQ(
        telemetry::toDbusTriggerAction("RedfishEvent"),
        "xyz.openbmc_project.Telemetry.Trigger.TriggerAction.LogToRedfishEventLog");
}

TEST(TriggerEnumConversion, ToDbusTriggerActionRejectsUnmappedRedfishActions)
{
    EXPECT_EQ(telemetry::toDbusTriggerAction("LogToLogService"), "");
    EXPECT_EQ(telemetry::toDbusTriggerAction(""), "");
}

TEST(TriggerEnumConversion, ToDbusSeverityMapsRedfishHealth)
{
    EXPECT_EQ(telemetry::toDbusSeverity("OK"),
              "xyz.openbmc_project.Telemetry.Trigger.Severity.OK");
    EXPECT_EQ(telemetry::toDbusSeverity("Warning"),
              "xyz.openbmc_project.Telemetry.Trigger.Severity.Warning");
    EXPECT_EQ(telemetry::toDbusSeverity("Critical"),
              "xyz.openbmc_project.Telemetry.Trigger.Severity.Critical");
}

TEST(TriggerEnumConversion, ToDbusSeverityRejectsUnmappedRedfishHealth)
{
    EXPECT_EQ(telemetry::toDbusSeverity("Fatal"), "");
    EXPECT_EQ(telemetry::toDbusSeverity(""), "");
}

TEST(TriggerEnumConversion, ToRedfishSeverityRejectsUnmappedDbusSeverities)
{
    EXPECT_EQ(telemetry::toRedfishSeverity(
                  "xyz.openbmc_project.Telemetry.Trigger.Severity.Unknown"),
              resource::Health::Invalid);
    EXPECT_EQ(telemetry::toRedfishSeverity(""), resource::Health::Invalid);
}

TEST(TriggerEnumConversion, ToRedfishThresholdNameRejectsUnmappedDbusTypes)
{
    EXPECT_EQ(telemetry::toRedfishThresholdName(
                  "xyz.openbmc_project.Telemetry.Trigger.Type.UpperFatal"),
              "");
    EXPECT_EQ(telemetry::toRedfishThresholdName(""), "");
}

TEST(TriggerEnumConversion, ToDbusActivationMapsRedfishActivations)
{
    EXPECT_EQ(telemetry::toDbusActivation("Either"),
              "xyz.openbmc_project.Telemetry.Trigger.Direction.Either");
    EXPECT_EQ(telemetry::toDbusActivation("Decreasing"),
              "xyz.openbmc_project.Telemetry.Trigger.Direction.Decreasing");
    EXPECT_EQ(telemetry::toDbusActivation("Increasing"),
              "xyz.openbmc_project.Telemetry.Trigger.Direction.Increasing");
}

TEST(TriggerEnumConversion, ToDbusActivationRejectsUnmappedRedfishActivations)
{
    EXPECT_EQ(telemetry::toDbusActivation("Disabled"), "");
    EXPECT_EQ(telemetry::toDbusActivation(""), "");
}

TEST(TriggerEnumConversion, ToRedfishActivationRejectsUnmappedDbusDirections)
{
    EXPECT_EQ(telemetry::toRedfishActivation(
                  "xyz.openbmc_project.Telemetry.Trigger.Direction.Disabled"),
              triggers::ThresholdActivation::Invalid);
    EXPECT_EQ(telemetry::toRedfishActivation(""),
              triggers::ThresholdActivation::Invalid);
}

TEST(TriggerEnumConversion, MappedValuesRoundTripThroughDbus)
{
    EXPECT_EQ(telemetry::toRedfishSeverity(telemetry::toDbusSeverity("OK")),
              resource::Health::OK);
    EXPECT_EQ(
        telemetry::toRedfishSeverity(telemetry::toDbusSeverity("Warning")),
        resource::Health::Warning);
    EXPECT_EQ(
        telemetry::toRedfishSeverity(telemetry::toDbusSeverity("Critical")),
        resource::Health::Critical);

    EXPECT_EQ(telemetry::toRedfishActivation(
                  telemetry::toDbusActivation("Increasing")),
              triggers::ThresholdActivation::Increasing);
    EXPECT_EQ(telemetry::toRedfishActivation(
                  telemetry::toDbusActivation("Decreasing")),
              triggers::ThresholdActivation::Decreasing);
    EXPECT_EQ(
        telemetry::toRedfishActivation(telemetry::toDbusActivation("Either")),
        triggers::ThresholdActivation::Either);

    EXPECT_EQ(telemetry::toRedfishTriggerAction(
                  telemetry::toDbusTriggerAction("RedfishEvent")),
              triggers::TriggerActionEnum::RedfishEvent);
    EXPECT_EQ(telemetry::toRedfishTriggerAction(
                  telemetry::toDbusTriggerAction("RedfishMetricReport")),
              triggers::TriggerActionEnum::RedfishMetricReport);
}

} // namespace
} // namespace redfish
