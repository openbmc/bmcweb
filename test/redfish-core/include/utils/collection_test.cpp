// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "async_resp.hpp"
#include "utils/collection.hpp"

#include <boost/system/errc.hpp>
#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>

#include <memory>

#include <gtest/gtest.h>

namespace redfish::collection_util
{
namespace
{

TEST(CollectionUtil, GetJsonArrayAtConvertsValueToArray)
{
    nlohmann::json value = 123;
    nlohmann::json::array_t& arr = details::getJsonArrayAt(value);
    EXPECT_TRUE(value.is_array());
    EXPECT_TRUE(arr.empty());
}

TEST(CollectionUtil, GetJsonArrayAtCreatesNestedArray)
{
    nlohmann::json root;
    nlohmann::json::array_t& arr = details::getJsonArrayAt(
        root[nlohmann::json::json_pointer("/Links/Members")]);
    EXPECT_TRUE(root["Links"]["Members"].is_array());
    EXPECT_TRUE(arr.empty());
}

TEST(CollectionUtil, GetJsonArrayAtConvertsNonArrayToArray)
{
    nlohmann::json root;
    root["Links"]["Members"] = {{"unexpected", "value"}};

    nlohmann::json::array_t& arr = details::getJsonArrayAt(
        root[nlohmann::json::json_pointer("/Links/Members")]);

    EXPECT_TRUE(root["Links"]["Members"].is_array());
    EXPECT_TRUE(arr.empty());
}

TEST(CollectionUtil, GetJsonArrayPreservesExistingArray)
{
    nlohmann::json root;
    root["Members"] =
        nlohmann::json::array({{{"@odata.id", "/redfish/v1/Managers/bmc"}}});

    nlohmann::json::array_t& arr = getJsonArray(root, "Members");

    ASSERT_TRUE(root["Members"].is_array());
    EXPECT_EQ(arr.size(), 1);
    EXPECT_EQ(arr.front()["@odata.id"], "/redfish/v1/Managers/bmc");
}

TEST(CollectionUtil, HandleCollectionMembersIoErrorPreservesExistingMembers)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    asyncResp->res.jsonValue["Members"] = nlohmann::json::array(
        {{{"@odata.id",
           "/redfish/v1/UpdateService/SoftwareInventory/existing"}}});

    handleCollectionMembers(
        asyncResp,
        boost::urls::url("/redfish/v1/UpdateService/SoftwareInventory"),
        nlohmann::json::json_pointer("/Members"),
        make_error_code(boost::system::errc::io_error), {});

    ASSERT_TRUE(asyncResp->res.jsonValue["Members"].is_array());
    EXPECT_EQ(asyncResp->res.jsonValue["Members"].size(), 1);
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("Members@odata.count"));
}

TEST(CollectionUtil, HandleCollectionMembersEmptyJsonKeyReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    handleCollectionMembers(
        asyncResp,
        boost::urls::url("/redfish/v1/UpdateService/SoftwareInventory"),
        nlohmann::json::json_pointer(), {}, {});

    EXPECT_EQ(asyncResp->res.resultInt(), 500);
    EXPECT_TRUE(asyncResp->res.jsonValue.contains("error"));
}

TEST(CollectionUtil, HandleCollectionMembersAppendsMembersAndUpdatesCount)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    asyncResp->res.jsonValue["Members"] = nlohmann::json::array(
        {{{"@odata.id",
           "/redfish/v1/UpdateService/SoftwareInventory/existing"}}});

    dbus::utility::MapperGetSubTreePathsResponse objects = {
        "/xyz/openbmc_project/software/C",
        "/xyz/openbmc_project/software/B",
        "/xyz/openbmc_project/software/A",
    };

    handleCollectionMembers(
        asyncResp,
        boost::urls::url("/redfish/v1/UpdateService/SoftwareInventory"),
        nlohmann::json::json_pointer("/Members"), {}, objects);

    const nlohmann::json& members = asyncResp->res.jsonValue["Members"];
    ASSERT_TRUE(members.is_array());
    EXPECT_EQ(members.size(), 4);
    EXPECT_EQ(asyncResp->res.jsonValue["Members@odata.count"], 4);
    EXPECT_EQ(members[0]["@odata.id"],
              "/redfish/v1/UpdateService/SoftwareInventory/A");
    EXPECT_EQ(members[1]["@odata.id"],
              "/redfish/v1/UpdateService/SoftwareInventory/B");
    EXPECT_EQ(members[2]["@odata.id"],
              "/redfish/v1/UpdateService/SoftwareInventory/C");
    EXPECT_EQ(members[3]["@odata.id"],
              "/redfish/v1/UpdateService/SoftwareInventory/existing");
}

TEST(CollectionUtil, HandleCollectionMembersSkipsObjectsWithEmptyLeaf)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::MapperGetSubTreePathsResponse objects = {
        "/xyz/openbmc_project/software/",
        "/xyz/openbmc_project/software/A",
    };

    handleCollectionMembers(
        asyncResp,
        boost::urls::url("/redfish/v1/UpdateService/SoftwareInventory"),
        nlohmann::json::json_pointer("/Members"), {}, objects);

    const nlohmann::json& members = asyncResp->res.jsonValue["Members"];
    ASSERT_TRUE(members.is_array());
    ASSERT_EQ(members.size(), 1);
    EXPECT_EQ(members[0]["@odata.id"],
              "/redfish/v1/UpdateService/SoftwareInventory/A");
    EXPECT_EQ(asyncResp->res.jsonValue["Members@odata.count"], 1);
}

} // namespace
} // namespace redfish::collection_util
