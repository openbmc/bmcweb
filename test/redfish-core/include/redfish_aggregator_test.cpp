#include "redfish_aggregator.hpp"

#include <nlohmann/json.hpp>

#include <gtest/gtest.h> // IWYU pragma: keep

namespace redfish
{
namespace
{

TEST(IsPropertyUri, SupportedPropertyReturnsTrue)
{
    EXPECT_TRUE(isPropertyUri("@Redfish.ActionInfo"));
    EXPECT_TRUE(isPropertyUri("@odata.id"));
    EXPECT_TRUE(isPropertyUri("Image"));
    EXPECT_TRUE(isPropertyUri("MetricProperty"));
    EXPECT_TRUE(isPropertyUri("TaskMonitor"));
    EXPECT_TRUE(isPropertyUri("target"));
}

TEST(IsPropertyUri, CaseInsensitiveURIReturnsTrue)
{
    EXPECT_TRUE(isPropertyUri("AdditionalDataURI"));
    EXPECT_TRUE(isPropertyUri("DataSourceUri"));
    EXPECT_TRUE(isPropertyUri("uri"));
    EXPECT_TRUE(isPropertyUri("URI"));
}

TEST(IsPropertyUri, SpeificallyIgnoredPropertyReturnsFalse)
{
    EXPECT_FALSE(isPropertyUri("@odata.context"));
    EXPECT_FALSE(isPropertyUri("Destination"));
    EXPECT_FALSE(isPropertyUri("HostName"));
    EXPECT_FALSE(isPropertyUri("OriginOfCondition"));
}

TEST(IsPropertyUri, UnsupportedPropertyReturnsFalse)
{
    EXPECT_FALSE(isPropertyUri("Name"));
    EXPECT_FALSE(isPropertyUri("Health"));
    EXPECT_FALSE(isPropertyUri("Id"));
}

TEST(addPrefixToItem, ValidURIs)
{
    nlohmann::json jsonRequest;
    constexpr std::array validRoots{"Cables",
                                    "Chassis",
                                    "Fabrics",
                                    "PowerEquipment/FloorPDUs",
                                    "Systems",
                                    "TaskService/Tasks",
                                    "TelemetryService/LogService/Entries",
                                    "UpdateService/SoftwareInventory"};

    // We're only testing prefix fixing so it's alright that some of the
    // resulting URIs will not actually be possible as defined by the schema
    constexpr std::array validIDs{"1",
                                  "1/",
                                  "Test",
                                  "Test/",
                                  "Extra_Test",
                                  "Extra_Test/",
                                  "Extra_Test/Sensors",
                                  "Extra_Test/Sensors/",
                                  "Extra_Test/Sensors/power_sensor",
                                  "Extra_Test/Sensors/power_sensor/"};

    // Construct URIs which should have prefix fixing applied
    for (const auto& root : validRoots)
    {
        for (const auto& id : validIDs)
        {
            std::string initial("/redfish/v1/" + std::string(root) + "/");
            std::string correct(initial + "asdfjkl_" + std::string(id));
            initial += id;
            jsonRequest["@odata.id"] = initial;
            addPrefixToItem(jsonRequest["@odata.id"], "asdfjkl");
            EXPECT_EQ(jsonRequest["@odata.id"], correct);
        }
    }
}

TEST(addPrefixToItem, UnsupportedURIs)
{
    nlohmann::json jsonRequest;
    constexpr std::array invalidRoots{
        "FakeCollection",           "JsonSchemas",
        "PowerEquipment",           "TaskService",
        "TelemetryService/Entries", "UpdateService"};

    constexpr std::array validIDs{"1",
                                  "1/",
                                  "Test",
                                  "Test/",
                                  "Extra_Test",
                                  "Extra_Test/",
                                  "Extra_Test/Sensors",
                                  "Extra_Test/Sensors/",
                                  "Extra_Test/Sensors/power_sensor",
                                  "Extra_Test/Sensors/power_sensor/"};

    // Construct URIs which should NOT have prefix fixing applied
    for (const auto& root : invalidRoots)
    {
        for (const auto& id : validIDs)
        {
            std::string initial("/redfish/v1/" + std::string(root) + "/");
            std::string correct(initial + "asdfjkl_" + std::string(id));
            initial += id;
            jsonRequest["@odata.id"] = initial;
            addPrefixToItem(jsonRequest["@odata.id"], "asdfjkl");
            EXPECT_EQ(jsonRequest["@odata.id"], initial);
        }
    }
}

TEST(addPrefixToItem, TopLevelCollections)
{
    nlohmann::json jsonRequest;
    constexpr std::array validRoots{"Cables",
                                    "Chassis/",
                                    "Fabrics",
                                    "JsonSchemas",
                                    "PowerEquipment/FloorPDUs",
                                    "Systems",
                                    "TaskService/Tasks",
                                    "TelemetryService/LogService/Entries",
                                    "TelemetryService/LogService/Entries/",
                                    "UpdateService/SoftwareInventory/"};

    // Construct URIs for top level collections.  Prefixes should NOT be
    // applied to any of the URIs
    for (const auto& root : validRoots)
    {
        std::string initial("/redfish/v1/" + std::string(root));
        jsonRequest["@odata.id"] = initial;
        addPrefixToItem(jsonRequest["@odata.id"], "perfix");
        EXPECT_EQ(jsonRequest["@odata.id"], initial);
    }
}

TEST(addPrefixes, ParseJsonObject)
{
    nlohmann::json parameter;
    parameter["Name"] = "/redfish/v1/Chassis/fakeName";
    parameter["@odata.id"] = "/redfish/v1/Chassis/fakeChassis";

    addPrefixes(parameter, "abcd");
    EXPECT_EQ(parameter["Name"], "/redfish/v1/Chassis/fakeName");
    EXPECT_EQ(parameter["@odata.id"], "/redfish/v1/Chassis/abcd_fakeChassis");
}

TEST(addPrefixes, ParseJsonArray)
{
    nlohmann::json array = nlohmann::json::parse(R"(
    {
      "Conditions": [
        {
          "Message": "This is a test",
          "@odata.id": "/redfish/v1/Chassis/TestChassis"
        },
        {
          "Message": "This is also a test",
          "@odata.id": "/redfish/v1/Chassis/TestChassis2"
        }
      ]
    }
    )",
                                                 nullptr, false);

    addPrefixes(array, "5B42");
    EXPECT_EQ(array["Conditions"][0]["@odata.id"],
              "/redfish/v1/Chassis/5B42_TestChassis");
    EXPECT_EQ(array["Conditions"][1]["@odata.id"],
              "/redfish/v1/Chassis/5B42_TestChassis2");
}

TEST(addPrefixes, ParseJsonObjectNestedArray)
{
    nlohmann::json objWithArray = nlohmann::json::parse(R"(
    {
      "Status": {
        "Conditions": [
          {
            "Message": "This is a test",
            "MessageId": "Test",
            "OriginOfCondition": {
              "@odata.id": "/redfish/v1/Chassis/TestChassis"
            },
            "Severity": "Critical"
          }
        ],
        "Health": "Critical",
        "State": "Enabled"
      }
    }
    )",
                                                        nullptr, false);

    addPrefixes(objWithArray, "5B42");
    nlohmann::json& array = objWithArray["Status"]["Conditions"];
    EXPECT_EQ(array[0]["OriginOfCondition"]["@odata.id"],
              "/redfish/v1/Chassis/5B42_TestChassis");
}

// Attempts to perform prefix fixing on a response with response code "result".
// Fixing should always occur
void assertProcessResponse(unsigned result)
{
    nlohmann::json jsonResp;
    jsonResp["@odata.id"] = "/redfish/v1/Chassis/TestChassis";
    jsonResp["Name"] = "Test";

    crow::Response resp;
    resp.body() =
        jsonResp.dump(2, ' ', true, nlohmann::json::error_handler_t::replace);
    resp.addHeader("Content-Type", "application/json");
    resp.result(result);

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    RedfishAggregator::processResponse("prefix", asyncResp, resp);

    EXPECT_EQ(asyncResp->res.jsonValue["Name"], "Test");
    EXPECT_EQ(asyncResp->res.jsonValue["@odata.id"],
              "/redfish/v1/Chassis/prefix_TestChassis");
    EXPECT_EQ(asyncResp->res.resultInt(), result);
}

TEST(processResponse, validResponseCodes)
{
    assertProcessResponse(100);
    assertProcessResponse(200);
    assertProcessResponse(204);
    assertProcessResponse(300);
    assertProcessResponse(404);
    assertProcessResponse(405);
    assertProcessResponse(500);
    assertProcessResponse(507);
}

} // namespace
} // namespace redfish
