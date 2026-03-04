// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "async_resp.hpp"
#include "utils/collection.hpp"

#include <boost/system/errc.hpp>
#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <set>
#include <string>
#include <vector>

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

TEST(CollectionUtil, GetJsonArrayByPointerCreatesNestedArray)
{
    nlohmann::json root;
    nlohmann::json::array_t& arr = getJsonArrayByPointer(
        root, nlohmann::json::json_pointer("/Links/Members"));
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

TEST(CollectionUtil, SortMembersByOdataIdSortsLexicographically)
{
    nlohmann::json::array_t members = {
        nlohmann::json(
            {{"@odata.id", "/redfish/v1/UpdateService/SoftwareInventory/C"}}),
        nlohmann::json(
            {{"@odata.id", "/redfish/v1/UpdateService/SoftwareInventory/A"}}),
        nlohmann::json(
            {{"@odata.id", "/redfish/v1/UpdateService/SoftwareInventory/B"}}),
    };

    sortMembersByOdataId(members);

    ASSERT_EQ(members.size(), 3);
    EXPECT_EQ(members[0]["@odata.id"],
              "/redfish/v1/UpdateService/SoftwareInventory/A");
    EXPECT_EQ(members[1]["@odata.id"],
              "/redfish/v1/UpdateService/SoftwareInventory/B");
    EXPECT_EQ(members[2]["@odata.id"],
              "/redfish/v1/UpdateService/SoftwareInventory/C");
}

TEST(CollectionUtil, SortMembersByOdataIdHandlesMissingOrInvalidIds)
{
    nlohmann::json::array_t members = {
        nlohmann::json({{"NotOdataId", "missing"}}),
        nlohmann::json({{"@odata.id", 42}}),
        nlohmann::json(
            {{"@odata.id", "/redfish/v1/UpdateService/SoftwareInventory/A"}}),
    };

    sortMembersByOdataId(members);

    ASSERT_EQ(members.size(), 3);
    EXPECT_EQ(members[2]["@odata.id"],
              "/redfish/v1/UpdateService/SoftwareInventory/A");
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
    EXPECT_EQ(asyncResp->res.jsonValue["Members@odata.count"], 1);
}

TEST(CollectionUtil, HandleCollectionMembersAppendsMembersAndUpdatesCount)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    asyncResp->res.jsonValue["Members"] = nlohmann::json::array(
        {{{"@odata.id",
           "/redfish/v1/UpdateService/SoftwareInventory/existing"}}});

    dbus::utility::MapperGetSubTreePathsResponse objects = {
        "/xyz/openbmc_project/software/A",
        "/xyz/openbmc_project/software/B",
        "/xyz/openbmc_project/software/A",
    };

    handleCollectionMembers(
        asyncResp,
        boost::urls::url("/redfish/v1/UpdateService/SoftwareInventory"),
        nlohmann::json::json_pointer("/Members"), {}, objects);

    const nlohmann::json& members = asyncResp->res.jsonValue["Members"];
    ASSERT_TRUE(members.is_array());
    EXPECT_EQ(asyncResp->res.jsonValue["Members@odata.count"], 3);

    std::set<std::string> ids;
    for (const auto& member : members)
    {
        ids.emplace(member["@odata.id"].get<std::string>());
    }
    EXPECT_NE(ids.find("/redfish/v1/UpdateService/SoftwareInventory/existing"),
              ids.end());
    EXPECT_NE(ids.find("/redfish/v1/UpdateService/SoftwareInventory/A"),
              ids.end());
    EXPECT_NE(ids.find("/redfish/v1/UpdateService/SoftwareInventory/B"),
              ids.end());
}

} // namespace
} // namespace redfish::collection_util
