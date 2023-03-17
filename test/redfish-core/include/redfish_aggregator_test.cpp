#include "async_resp.hpp"
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

TEST(addPrefixes, FixHttpHeadersInResponseBody)
{
    nlohmann::json taskResp = nlohmann::json::parse(R"(
    {
      "@odata.id": "/redfish/v1/TaskService/Tasks/0",
      "Name": "Task 0",
      "Payload": {
        "HttpHeaders": [
          "User-Agent: curl/7.87.0",
          "Accept: */*",
          "Host: 127.127.12.7",
          "Content-Length: 33",
          "Location: /redfish/v1/Managers/bmc/LogServices/Dump/Entries/0"
        ]
      },
      "PercentComplete": 100,
      "TaskMonitor": "/redfish/v1/TaskService/Tasks/0/Monitor",
      "TaskState": "Completed",
      "TaskStatus": "OK"
    }
    )",
                                                    nullptr, false);

    addPrefixes(taskResp, "5B247A");
    EXPECT_EQ(taskResp["@odata.id"], "/redfish/v1/TaskService/Tasks/5B247A_0");
    EXPECT_EQ(taskResp["TaskMonitor"],
              "/redfish/v1/TaskService/Tasks/5B247A_0/Monitor");
    nlohmann::json& httpHeaders = taskResp["Payload"]["HttpHeaders"];
    EXPECT_EQ(
        httpHeaders[4],
        "Location: /redfish/v1/Managers/5B247A_bmc/LogServices/Dump/Entries/0");
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
    resp.addHeader("Allow", "GET");
    resp.addHeader("Location", "/redfish/v1/Chassis/TestChassis");
    resp.addHeader("Link", "</redfish/v1/Test.json>; rel=describedby");
    resp.addHeader("Retry-After", "120");
    resp.result(result);

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    RedfishAggregator::processResponse("prefix", asyncResp, resp);

    EXPECT_EQ(asyncResp->res.getHeaderValue("Content-Type"),
              "application/json");
    EXPECT_EQ(asyncResp->res.getHeaderValue("Allow"), "GET");
    EXPECT_EQ(asyncResp->res.getHeaderValue("Location"),
              "/redfish/v1/Chassis/prefix_TestChassis");
    EXPECT_EQ(asyncResp->res.getHeaderValue("Link"), "");
    EXPECT_EQ(asyncResp->res.getHeaderValue("Retry-After"), "120");

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

TEST(processResponse, preserveHeaders)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    asyncResp->res.addHeader("OData-Version", "4.0");
    asyncResp->res.result(boost::beast::http::status::ok);

    crow::Response resp;
    resp.addHeader("OData-Version", "3.0");
    resp.addHeader(boost::beast::http::field::location,
                   "/redfish/v1/Chassis/Test");
    resp.result(boost::beast::http::status::too_many_requests); // 429

    RedfishAggregator::processResponse("prefix", asyncResp, resp);
    EXPECT_EQ(asyncResp->res.resultInt(), 429);
    EXPECT_EQ(asyncResp->res.getHeaderValue("OData-Version"), "4.0");
    EXPECT_EQ(asyncResp->res.getHeaderValue("Location"), "");

    asyncResp->res.result(boost::beast::http::status::ok);
    resp.result(boost::beast::http::status::bad_gateway); // 502

    RedfishAggregator::processResponse("prefix", asyncResp, resp);
    EXPECT_EQ(asyncResp->res.resultInt(), 502);
    EXPECT_EQ(asyncResp->res.getHeaderValue("OData-Version"), "4.0");
    EXPECT_EQ(asyncResp->res.getHeaderValue("Location"), "");
}

// Helper function to correctly populate a ComputerSystem collection response
void populateCollectionResponse(crow::Response& resp)
{
    nlohmann::json jsonResp = nlohmann::json::parse(R"(
    {
      "@odata.id": "/redfish/v1/Systems",
      "@odata.type": "#ComputerSystemCollection.ComputerSystemCollection",
      "Members": [
        {
          "@odata.id": "/redfish/v1/Systems/system"
        }
      ],
      "Members@odata.count": 1,
      "Name": "Computer System Collection"
    }
    )",
                                                    nullptr, false);

    resp.clear();
    // resp.body() =
    //     jsonResp.dump(2, ' ', true,
    //     nlohmann::json::error_handler_t::replace);
    resp.jsonValue = std::move(jsonResp);
    resp.addHeader("OData-Version", "4.0");
    resp.addHeader("Content-Type", "application/json");
    resp.result(boost::beast::http::status::ok);
}

void populateCollectionNotFound(crow::Response& resp)
{
    resp.clear();
    resp.addHeader("OData-Version", "4.0");
    resp.result(boost::beast::http::status::not_found);
}

// Used with the above functions to convert the response to appear like it's
// from a satellite which will not have a json component
void convertToSat(crow::Response& resp)
{
    resp.body() = resp.jsonValue.dump(2, ' ', true,
                                      nlohmann::json::error_handler_t::replace);
    resp.jsonValue.clear();
}

TEST(processCollectionResponse, localOnly)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    crow::Response resp;
    populateCollectionResponse(asyncResp->res);
    populateCollectionNotFound(resp);

    RedfishAggregator::processCollectionResponse("prefix", asyncResp, resp);
    EXPECT_EQ(asyncResp->res.getHeaderValue("OData-Version"), "4.0");
    EXPECT_EQ(asyncResp->res.resultInt(), 200);
    EXPECT_EQ(asyncResp->res.getHeaderValue("Content-Type"),
              "application/json");
    EXPECT_EQ(asyncResp->res.jsonValue["Members@odata.count"], 1);
    for (auto& member : asyncResp->res.jsonValue["Members"])
    {
        // There should only be one member
        EXPECT_EQ(member["@odata.id"], "/redfish/v1/Systems/system");
    }
}

TEST(processCollectionResponse, satelliteOnly)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    crow::Response resp;
    populateCollectionNotFound(asyncResp->res);
    populateCollectionResponse(resp);
    convertToSat(resp);

    RedfishAggregator::processCollectionResponse("prefix", asyncResp, resp);
    EXPECT_EQ(asyncResp->res.getHeaderValue("OData-Version"), "4.0");
    EXPECT_EQ(asyncResp->res.resultInt(), 200);
    EXPECT_EQ(asyncResp->res.getHeaderValue("Content-Type"),
              "application/json");
    EXPECT_EQ(asyncResp->res.jsonValue["Members@odata.count"], 1);
    for (auto& member : asyncResp->res.jsonValue["Members"])
    {
        // There should only be one member
        EXPECT_EQ(member["@odata.id"], "/redfish/v1/Systems/prefix_system");
    }
}

TEST(processCollectionResponse, bothExist)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    crow::Response resp;
    populateCollectionResponse(asyncResp->res);
    populateCollectionResponse(resp);
    convertToSat(resp);

    RedfishAggregator::processCollectionResponse("prefix", asyncResp, resp);
    EXPECT_EQ(asyncResp->res.getHeaderValue("OData-Version"), "4.0");
    EXPECT_EQ(asyncResp->res.resultInt(), 200);
    EXPECT_EQ(asyncResp->res.getHeaderValue("Content-Type"),
              "application/json");
    EXPECT_EQ(asyncResp->res.jsonValue["Members@odata.count"], 2);

    bool foundLocal = false;
    bool foundSat = false;
    for (const auto& member : asyncResp->res.jsonValue["Members"])
    {
        if (member["@odata.id"] == "/redfish/v1/Systems/system")
        {
            foundLocal = true;
        }
        else if (member["@odata.id"] == "/redfish/v1/Systems/prefix_system")
        {
            foundSat = true;
        }
    }
    EXPECT_TRUE(foundLocal);
    EXPECT_TRUE(foundSat);
}

TEST(processCollectionResponse, satelliteWrongContentHeader)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    crow::Response resp;
    populateCollectionResponse(asyncResp->res);
    populateCollectionResponse(resp);
    convertToSat(resp);

    // Ignore the satellite even though otherwise valid
    resp.addHeader("Content-Type", "");

    RedfishAggregator::processCollectionResponse("prefix", asyncResp, resp);
    EXPECT_EQ(asyncResp->res.getHeaderValue("OData-Version"), "4.0");
    EXPECT_EQ(asyncResp->res.resultInt(), 200);
    EXPECT_EQ(asyncResp->res.getHeaderValue("Content-Type"),
              "application/json");
    EXPECT_EQ(asyncResp->res.jsonValue["Members@odata.count"], 1);
    for (auto& member : asyncResp->res.jsonValue["Members"])
    {
        EXPECT_EQ(member["@odata.id"], "/redfish/v1/Systems/system");
    }
}

TEST(processCollectionResponse, neitherExist)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    crow::Response resp;
    populateCollectionNotFound(asyncResp->res);
    populateCollectionNotFound(resp);
    convertToSat(resp);

    RedfishAggregator::processCollectionResponse("prefix", asyncResp, resp);
    EXPECT_EQ(asyncResp->res.getHeaderValue("OData-Version"), "4.0");
    EXPECT_EQ(asyncResp->res.resultInt(), 404);
    EXPECT_EQ(asyncResp->res.getHeaderValue("Content-Type"), "");
}

TEST(processCollectionResponse, preserveHeaders)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    crow::Response resp;
    populateCollectionNotFound(asyncResp->res);
    populateCollectionResponse(resp);
    convertToSat(resp);

    resp.addHeader("OData-Version", "3.0");
    resp.addHeader(boost::beast::http::field::location,
                   "/redfish/v1/Chassis/Test");

    // We skip processing collection responses that have a 429 or 502 code
    resp.result(boost::beast::http::status::too_many_requests); // 429
    RedfishAggregator::processCollectionResponse("prefix", asyncResp, resp);
    EXPECT_EQ(asyncResp->res.resultInt(), 404);
    EXPECT_EQ(asyncResp->res.getHeaderValue("OData-Version"), "4.0");
    EXPECT_EQ(asyncResp->res.getHeaderValue("Location"), "");

    resp.result(boost::beast::http::status::bad_gateway); // 502
    RedfishAggregator::processCollectionResponse("prefix", asyncResp, resp);
    EXPECT_EQ(asyncResp->res.resultInt(), 404);
    EXPECT_EQ(asyncResp->res.getHeaderValue("OData-Version"), "4.0");
    EXPECT_EQ(asyncResp->res.getHeaderValue("Location"), "");
}
void assertProcessResponseContentType(std::string_view contentType)
{
    crow::Response resp;
    resp.body() = "responseBody";
    resp.addHeader("Content-Type", contentType);
    resp.addHeader("Location", "/redfish/v1/Chassis/TestChassis");
    resp.addHeader("Link", "metadataLink");
    resp.addHeader("Retry-After", "120");

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    RedfishAggregator::processResponse("prefix", asyncResp, resp);
    EXPECT_EQ(asyncResp->res.getHeaderValue("Content-Type"), contentType);
    EXPECT_EQ(asyncResp->res.getHeaderValue("Location"),
              "/redfish/v1/Chassis/prefix_TestChassis");
    EXPECT_EQ(asyncResp->res.getHeaderValue("Link"), "");
    EXPECT_EQ(asyncResp->res.getHeaderValue("Retry-After"), "120");
    EXPECT_EQ(asyncResp->res.body(), "responseBody");
}

TEST(processResponse, DifferentContentType)
{
    assertProcessResponseContentType("application/xml");
    assertProcessResponseContentType("application/yaml");
    assertProcessResponseContentType("text/event-stream");
    assertProcessResponseContentType(";charset=utf-8");
}

} // namespace
} // namespace redfish
