// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "generated/enums/port.hpp"
#include "network_adapter.hpp"

#include <nlohmann/json.hpp>

#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

constexpr const char* modePrefix =
    "xyz.openbmc_project.Network.LLDP.Configuration.Mode.";
constexpr const char* subtypePrefix =
    "xyz.openbmc_project.Network.LLDP.TLVs.IdSubtype.";
constexpr const char* capabilityPrefix =
    "xyz.openbmc_project.Network.LLDP.TLVs.SystemCapability.";

TEST(LldpEnabledFrom, OffOnlyWhenNeitherDirectionRuns)
{
    EXPECT_FALSE(lldpEnabledFrom(std::string(modePrefix) + "Disabled",
                                 std::string(modePrefix) + "Disabled"));
}

// A port that only announces itself still speaks the protocol, so either
// direction alone is enough for Redfish to call the agent enabled.
TEST(LldpEnabledFrom, OnWhenEitherDirectionRuns)
{
    EXPECT_TRUE(lldpEnabledFrom(std::string(modePrefix) + "All",
                                std::string(modePrefix) + "Disabled"));
    EXPECT_TRUE(lldpEnabledFrom(std::string(modePrefix) + "Disabled",
                                std::string(modePrefix) + "Mandatory"));
    EXPECT_TRUE(lldpEnabledFrom(std::string(modePrefix) + "All",
                                std::string(modePrefix) + "All"));
}

// A value from outside the interface is not Disabled, and reporting the agent
// as enabled for it is the safer of the two wrong answers: it points a
// consumer at the mode rather than hiding it.
TEST(LldpEnabledFrom, TreatsAnUnknownModeAsRunning)
{
    EXPECT_TRUE(lldpEnabledFrom("something.else", "something.else"));
}

TEST(ToIdSubtype, ReadsTheNamesTheInterfaceDefines)
{
    EXPECT_EQ(toIdSubtype(std::string(subtypePrefix) + "MacAddr"),
              port::IEEE802IdSubtype::MacAddr);
    EXPECT_EQ(toIdSubtype(std::string(subtypePrefix) + "NetworkAddr"),
              port::IEEE802IdSubtype::NetworkAddr);
    EXPECT_EQ(toIdSubtype(std::string(subtypePrefix) + "NotTransmitted"),
              port::IEEE802IdSubtype::NotTransmitted);
}

// A name is matched rather than trimmed, so a value either side stops
// recognising is reported as such instead of being passed through.
TEST(ToIdSubtype, RejectsWhatItDoesNotRecognise)
{
    EXPECT_EQ(toIdSubtype("MacAddr"), port::IEEE802IdSubtype::Invalid);
    EXPECT_EQ(toIdSubtype(std::string(subtypePrefix) + "NoSuchSubtype"),
              port::IEEE802IdSubtype::Invalid);
    EXPECT_EQ(toIdSubtype(""), port::IEEE802IdSubtype::Invalid);
}

TEST(ToSystemCapability, ReadsTheNamesTheInterfaceDefines)
{
    EXPECT_EQ(toSystemCapability(std::string(capabilityPrefix) + "Bridge"),
              port::LLDPSystemCapabilities::Bridge);
    EXPECT_EQ(toSystemCapability(std::string(capabilityPrefix) + "Router"),
              port::LLDPSystemCapabilities::Router);
}

TEST(ToSystemCapability, RejectsWhatItDoesNotRecognise)
{
    EXPECT_EQ(toSystemCapability("Bridge"),
              port::LLDPSystemCapabilities::Invalid);
    EXPECT_EQ(toSystemCapability(std::string(capabilityPrefix) + "NoSuchThing"),
              port::LLDPSystemCapabilities::Invalid);
}

// A subtype only means something next to the identifier it describes, so one
// the frame did not carry leaves the property out altogether.
TEST(ReportSubtype, LeavesTheFieldOutWhenTheFrameCarriedNone)
{
    nlohmann::json frame = nlohmann::json::object();

    reportSubtype(frame, "ChassisIdSubtype", std::nullopt);

    EXPECT_FALSE(frame.contains("ChassisIdSubtype"));
}

TEST(ReportSubtype, ReportsASubtypeTheFrameCarried)
{
    nlohmann::json frame = nlohmann::json::object();

    reportSubtype(frame, "ChassisIdSubtype",
                  std::string(subtypePrefix) + "MacAddr");

    EXPECT_EQ(frame["ChassisIdSubtype"],
              nlohmann::json(port::IEEE802IdSubtype::MacAddr));
}

// Both of these mean the resource has nothing to say: the enumeration has no
// member to stand for them, and inventing one would put a value in the
// resource that the schema does not define.
TEST(ReportSubtype, ReportsNullForNotTransmittedAndForTheUnrecognised)
{
    nlohmann::json frame = nlohmann::json::object();

    reportSubtype(frame, "ChassisIdSubtype",
                  std::string(subtypePrefix) + "NotTransmitted");
    reportSubtype(frame, "PortIdSubtype",
                  std::string(subtypePrefix) + "NoSuchSubtype");

    EXPECT_TRUE(frame["ChassisIdSubtype"].is_null());
    EXPECT_TRUE(frame["PortIdSubtype"].is_null());
}

} // namespace
} // namespace redfish
