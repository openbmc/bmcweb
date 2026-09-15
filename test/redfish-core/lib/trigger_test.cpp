// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "generated/enums/resource.hpp"
#include "generated/enums/triggers.hpp"
#include "trigger.hpp"

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(TriggerEnumConversion, ToRedfishTriggerActionMapsDbusActions)
{
    EXPECT_EQ(
        telemetry::toRedfishTriggerAction(
            "xyz.openbmc_project.Telemetry.Trigger.TriggerAction.UpdateReport"),
        triggers::TriggerActionEnum::RedfishMetricReport);
    EXPECT_EQ(
        telemetry::toRedfishTriggerAction(
            "xyz.openbmc_project.Telemetry.Trigger.TriggerAction.LogToRedfishEventLog"),
        triggers::TriggerActionEnum::RedfishEvent);
}

TEST(TriggerEnumConversion, ToRedfishTriggerActionRejectsUnmappedDbusActions)
{
    EXPECT_EQ(
        telemetry::toRedfishTriggerAction(
            "xyz.openbmc_project.Telemetry.Trigger.TriggerAction.LogToJournal"),
        triggers::TriggerActionEnum::Invalid);
    EXPECT_EQ(telemetry::toRedfishTriggerAction("RedfishMetricReport"),
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
    EXPECT_EQ(
        telemetry::toDbusTriggerAction(
            "xyz.openbmc_project.Telemetry.Trigger.TriggerAction.UpdateReport"),
        "");
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
    EXPECT_EQ(telemetry::toDbusSeverity("ok"), "");
    EXPECT_EQ(telemetry::toDbusSeverity("Fatal"), "");
    EXPECT_EQ(telemetry::toDbusSeverity(""), "");
}

TEST(TriggerEnumConversion, ToRedfishSeverityMapsDbusSeverities)
{
    EXPECT_EQ(telemetry::toRedfishSeverity(
                  "xyz.openbmc_project.Telemetry.Trigger.Severity.OK"),
              resource::Health::OK);
    EXPECT_EQ(telemetry::toRedfishSeverity(
                  "xyz.openbmc_project.Telemetry.Trigger.Severity.Warning"),
              resource::Health::Warning);
    EXPECT_EQ(telemetry::toRedfishSeverity(
                  "xyz.openbmc_project.Telemetry.Trigger.Severity.Critical"),
              resource::Health::Critical);
}

TEST(TriggerEnumConversion, ToRedfishSeverityRejectsUnmappedDbusSeverities)
{
    EXPECT_EQ(telemetry::toRedfishSeverity(
                  "xyz.openbmc_project.Telemetry.Trigger.Severity.Unknown"),
              resource::Health::Invalid);
    EXPECT_EQ(telemetry::toRedfishSeverity("OK"), resource::Health::Invalid);
    EXPECT_EQ(telemetry::toRedfishSeverity(""), resource::Health::Invalid);
}

TEST(TriggerEnumConversion, ToRedfishThresholdNameMapsDbusTypes)
{
    EXPECT_EQ(telemetry::toRedfishThresholdName(
                  "xyz.openbmc_project.Telemetry.Trigger.Type.UpperCritical"),
              "UpperCritical");
    EXPECT_EQ(telemetry::toRedfishThresholdName(
                  "xyz.openbmc_project.Telemetry.Trigger.Type.LowerCritical"),
              "LowerCritical");
    EXPECT_EQ(telemetry::toRedfishThresholdName(
                  "xyz.openbmc_project.Telemetry.Trigger.Type.UpperWarning"),
              "UpperWarning");
    EXPECT_EQ(telemetry::toRedfishThresholdName(
                  "xyz.openbmc_project.Telemetry.Trigger.Type.LowerWarning"),
              "LowerWarning");
}

TEST(TriggerEnumConversion, ToRedfishThresholdNameRejectsUnmappedDbusTypes)
{
    EXPECT_EQ(telemetry::toRedfishThresholdName(
                  "xyz.openbmc_project.Telemetry.Trigger.Type.UpperFatal"),
              "");
    EXPECT_EQ(telemetry::toRedfishThresholdName(
                  "xyz.openbmc_project.Telemetry.Trigger.Severity.Critical"),
              "");
    EXPECT_EQ(telemetry::toRedfishThresholdName("UpperCritical"), "");
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
    EXPECT_EQ(telemetry::toDbusActivation(
                  "xyz.openbmc_project.Telemetry.Trigger.Direction.Either"),
              "");
    EXPECT_EQ(telemetry::toDbusActivation(""), "");
}

TEST(TriggerEnumConversion, ToRedfishActivationMapsDbusDirections)
{
    EXPECT_EQ(telemetry::toRedfishActivation(
                  "xyz.openbmc_project.Telemetry.Trigger.Direction.Either"),
              triggers::ThresholdActivation::Either);
    EXPECT_EQ(telemetry::toRedfishActivation(
                  "xyz.openbmc_project.Telemetry.Trigger.Direction.Decreasing"),
              triggers::ThresholdActivation::Decreasing);
    EXPECT_EQ(telemetry::toRedfishActivation(
                  "xyz.openbmc_project.Telemetry.Trigger.Direction.Increasing"),
              triggers::ThresholdActivation::Increasing);
}

TEST(TriggerEnumConversion, ToRedfishActivationRejectsUnmappedDbusDirections)
{
    EXPECT_EQ(telemetry::toRedfishActivation(
                  "xyz.openbmc_project.Telemetry.Trigger.Direction.Disabled"),
              triggers::ThresholdActivation::Invalid);
    EXPECT_EQ(telemetry::toRedfishActivation("Either"),
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
