#include "redfish_aggregator.hpp"

#include <gtest/gtest.h> // IWYU pragma: keep

namespace redfish
{

TEST(IsPropertyUri, SupportedPropertyReturnsTrue)
{
    EXPECT_TRUE(isPropertyUri("@Redfish.ActionInfo"));
    EXPECT_TRUE(isPropertyUri("@odata.id"));
    EXPECT_TRUE(isPropertyUri("Image"));
    EXPECT_TRUE(isPropertyUri("MetricProperty"));
    EXPECT_TRUE(isPropertyUri("OriginOfCondition"));
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
} // namespace redfish
