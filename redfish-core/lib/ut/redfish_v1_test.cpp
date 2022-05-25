#include "bmcweb_config.h"

#include "http_request.hpp"
#include "include/async_resp.hpp"
#include "nlohmann/json.hpp"
#include "redfish-core/lib/redfish_v1.hpp"

#include <memory>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

void assertCollectionMembers(nlohmann::json& members)
{
    const nlohmann::json::array_t& arr =
        members.get_ref<const nlohmann::json::array_t&>();
    for (const nlohmann::json& member : arr)
    {
        EXPECT_EQ(member.size(), 1);
        EXPECT_THAT(member["@odata.id"].get<std::string>(),
                    ::testing::StartsWith("/redfish/v1/JsonSchemas/"));
    }
}

TEST(RedfishV1, JsonSchemasCollection)
{
    nlohmann::json json;
    redfish::jsonSchemaResponseGet(json);

    EXPECT_EQ(json["@odata.id"], "/redfish/v1/JsonSchemas");
    EXPECT_EQ(
        json["@odata.context"],
        "/redfish/v1/$metadata#JsonSchemaFileCollection.JsonSchemaFileCollection");
    EXPECT_EQ(json["@odata.type"],
              "#JsonSchemaFileCollection.JsonSchemaFileCollection");

    EXPECT_EQ(json["Name"], "JsonSchemaFile Collection");
    EXPECT_EQ(json["Description"], "Collection of JsonSchemaFiles");

    EXPECT_GT(json["Members"].size(), 0);
    assertCollectionMembers(json["Members"]);
}
