// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "utils/systems_utils.hpp"

#include <nlohmann/json.hpp>

#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(SystemsUtils, IndexMatchingObjectPath)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    std::string objectPath;
    std::string service;

    const dbus::utility::MapperGetSubTreeResponse subtree = {
        {"/xyz/openbmc_project/control/host1",
         {{"xyz.openbmc_project.Settings", {}}}},
        {"/xyz/openbmc_project/control/host2/policy/TPMEnable",
         {{"xyz.openbmc_project.Settings", {}}}},
        {"/xyz/openbmc_project/control/host10/policy/TPMEnable",
         {{"xyz.openbmc_project.Settings", {}}}},
        {"/xyz/openbmc_project/control/host999/",
         {{"xyz.openbmc_project.Settings", {}}}}};

    EXPECT_TRUE(systems_utils::indexMatchingSubTreeMapObjectPath(
        asyncResp, 1, subtree, objectPath, service));
    EXPECT_TRUE(systems_utils::indexMatchingSubTreeMapObjectPath(
        asyncResp, 2, subtree, objectPath, service));
    EXPECT_TRUE(systems_utils::indexMatchingSubTreeMapObjectPath(
        asyncResp, 10, subtree, objectPath, service));
    EXPECT_TRUE(systems_utils::indexMatchingSubTreeMapObjectPath(
        asyncResp, 999, subtree, objectPath, service));
    EXPECT_FALSE(systems_utils::indexMatchingSubTreeMapObjectPath(
        asyncResp, 100, subtree, objectPath, service));
    EXPECT_FALSE(systems_utils::indexMatchingSubTreeMapObjectPath(
        asyncResp, 11, subtree, objectPath, service));
    EXPECT_FALSE(systems_utils::indexMatchingSubTreeMapObjectPath(
        asyncResp, 0, subtree, objectPath, service));

    systems_utils::indexMatchingSubTreeMapObjectPath(asyncResp, 1, subtree,
                                                     objectPath, service);
    EXPECT_EQ(objectPath, "/xyz/openbmc_project/control/host1");
    EXPECT_EQ(service, "xyz.openbmc_project.Settings");

    systems_utils::indexMatchingSubTreeMapObjectPath(asyncResp, 10, subtree,
                                                     objectPath, service);
    EXPECT_EQ(objectPath,
              "/xyz/openbmc_project/control/host10/policy/TPMEnable");

    systems_utils::indexMatchingSubTreeMapObjectPath(asyncResp, 999, subtree,
                                                     objectPath, service);
    EXPECT_EQ(objectPath, "/xyz/openbmc_project/control/host999/");
}
} // namespace
} // namespace redfish
