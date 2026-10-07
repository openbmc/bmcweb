// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "bmcweb_config.h"

#include "generated/enums/resource.hpp"
#include "utils/sw_utils.hpp"

#include <sdbusplus/message/native_types.hpp>

#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace redfish::sw_util
{
namespace
{

TEST(SwUtils, FunctionalSoftwarePath)
{
    if constexpr (BMCWEB_REDFISH_UPDATESERVICE_USE_DBUS)
    {
        EXPECT_EQ(getFunctionalSoftwarePath(bmcPurpose),
                  sdbusplus::object_path(
                      "/xyz/openbmc_project/software/bmc/functional"));
        EXPECT_EQ(getFunctionalSoftwarePath(biosPurpose),
                  sdbusplus::object_path(
                      "/xyz/openbmc_project/software/bios/functional"));
    }
    else
    {
        EXPECT_EQ(
            getFunctionalSoftwarePath(bmcPurpose),
            sdbusplus::object_path("/xyz/openbmc_project/software/functional"));
        EXPECT_EQ(
            getFunctionalSoftwarePath(biosPurpose),
            sdbusplus::object_path("/xyz/openbmc_project/software/functional"));
    }
}

TEST(SwUtils, UnknownPurposeHasNoFunctionalPath)
{
    EXPECT_EQ(getFunctionalSoftwarePath(""), std::nullopt);
    EXPECT_EQ(getFunctionalSoftwarePath(
                  "xyz.openbmc_project.Software.Version.VersionPurpose.Other"),
              std::nullopt);
}

TEST(SwUtils, ActivationToState)
{
    EXPECT_EQ(getRedfishSwState(
                  "xyz.openbmc_project.Software.Activation.Activations.Active"),
              resource::State::Enabled);
    EXPECT_EQ(
        getRedfishSwState(
            "xyz.openbmc_project.Software.Activation.Activations.Activating"),
        resource::State::Updating);
    EXPECT_EQ(
        getRedfishSwState(
            "xyz.openbmc_project.Software.Activation.Activations.StandbySpare"),
        resource::State::StandbySpare);
}

TEST(SwUtils, UnmappedActivationIsDisabled)
{
    EXPECT_EQ(getRedfishSwState(""), resource::State::Disabled);
    EXPECT_EQ(getRedfishSwState(
                  "xyz.openbmc_project.Software.Activation.Activations.Ready"),
              resource::State::Disabled);
    EXPECT_EQ(getRedfishSwState(
                  "xyz.openbmc_project.Software.Activation.Activations.Failed"),
              resource::State::Disabled);
}

TEST(SwUtils, ActivationToHealth)
{
    EXPECT_EQ(getRedfishSwHealth(
                  "xyz.openbmc_project.Software.Activation.Activations.Active"),
              resource::Health::OK);
    EXPECT_EQ(
        getRedfishSwHealth(
            "xyz.openbmc_project.Software.Activation.Activations.Activating"),
        resource::Health::OK);
    EXPECT_EQ(getRedfishSwHealth(
                  "xyz.openbmc_project.Software.Activation.Activations.Ready"),
              resource::Health::OK);
}

TEST(SwUtils, UnmappedActivationIsWarning)
{
    EXPECT_EQ(getRedfishSwHealth(""), resource::Health::Warning);
    EXPECT_EQ(getRedfishSwHealth(
                  "xyz.openbmc_project.Software.Activation.Activations.Failed"),
              resource::Health::Warning);
    EXPECT_EQ(
        getRedfishSwHealth(
            "xyz.openbmc_project.Software.Activation.Activations.StandbySpare"),
        resource::Health::Warning);
}

} // namespace
} // namespace redfish::sw_util
