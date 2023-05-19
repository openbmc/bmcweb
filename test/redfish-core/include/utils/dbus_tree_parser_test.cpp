#include "utils/dbus_tree_parser.hpp"

#include <string>

#include <gtest/gtest.h> // IWYU pragma: keep

// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
// IWYU pragma: no_include "gtest/gtest_pred_impl.h"

namespace redfish
{
namespace
{

auto make_entries(auto path)
{
    DBusPropertiesMap licence_entry = {
        DBusPropertiesMap::value_type{"Name", DbusVariantType("some name")},
        {"AuthDeviceNumber", uint32_t(32)},
        {"ExpirationTime", uint64_t(3434324234)},
        {"SampleVec",
         AssociationsValType{{"he", "ll", "o"}, {"hi", "by", "e"}}},
        {"SampleBool", true},
        {"SerialNumber", "3434324234"}};
    DBusPropertiesMap availability_entry = {DBusPropertiesMap::value_type{
        "Available", DbusVariantType(std::vector<uint32_t>{1, 2})}};
    return ManagedObjectType::value_type{
        sdbusplus::message::object_path(path),
        {{"xyz.openbmc_project.License.Entry.LicenseEntry", licence_entry},
         {"xyz.openbmc_project.State.Decorator.Availability",
          availability_entry}}};
}
TEST(TestParserPositive, PositiveCaseWithSuccess)
{
    auto entry1 = make_entries("/xyz/openbmc_project/license/entry1/");
    auto entry2 = make_entries("/xyz/openbmc_project/license/entry2/");
    ManagedObjectType resp = {entry1, entry2};

    struct Extraction_Handlers : DbusBaseHandler
    {
        std::vector<handler_pair> LicenseExtractor;
        std::vector<handler_pair> AvailabilityExtractor;
        Extraction_Handlers()
        {
            LicenseExtractor.emplace_back("Name", string_node_mapper());
            LicenseExtractor.emplace_back("AuthDeviceNumber",
                                          uint32_t_node_mapper());
            LicenseExtractor.emplace_back("SerialNumber", string_node_mapper());
            LicenseExtractor.emplace_back("SampleVec",
                                          association_val_type_node_mapper());
            handlers.emplace("xyz.openbmc_project.License.Entry.LicenseEntry",
                             make_extractor(LicenseExtractor));

            AvailabilityExtractor.emplace_back(
                "Available", vector_node_mapper<std::uint32_t>());
            handlers.emplace("xyz.openbmc_project.State.Decorator.Availability",
                             make_extractor(AvailabilityExtractor));
        }
    };
    Extraction_Handlers extraction_handlers;
    DbusTreeParser parser(resp, extraction_handlers, true);
    bool errorfound = false;
    json result;
    parser.on_error_message(
              [&](auto&& ) {
        errorfound = true;
    }).on_success([&](auto&& summary) {
          result = summary;
      }).parse();
    ASSERT_NE(errorfound, true);
    auto expected = R"({
  "/xyz/openbmc_project/license/entry1/": {
    "xyz.openbmc_project.License.Entry.LicenseEntry": {
      "AuthDeviceNumber": 32,
      "Name": "some name",
      "SampleVec": [
        [
          "he",
          "ll",
          "o"
        ],
        [
          "hi",
          "by",
          "e"
        ]
      ],
      "SerialNumber": "3434324234"
    },
    "xyz.openbmc_project.State.Decorator.Availability": {
      "Available": [
        1,
        2
      ]
    }
  },
  "/xyz/openbmc_project/license/entry2/": {
    "xyz.openbmc_project.License.Entry.LicenseEntry": {
      "AuthDeviceNumber": 32,
      "Name": "some name",
      "SampleVec": [
        [
          "he",
          "ll",
          "o"
        ],
        [
          "hi",
          "by",
          "e"
        ]
      ],
      "SerialNumber": "3434324234"
    },
    "xyz.openbmc_project.State.Decorator.Availability": {
      "Available": [
        1,
        2
      ]
    }
  }
})"_json;
    ASSERT_EQ(expected, result);
}

TEST(TestParserNegative, ParsetWithErrorMessage) {
    auto entry1 = make_entries("/xyz/openbmc_project/license/entry1/");
    auto entry2 = make_entries("/xyz/openbmc_project/license/entry2/");
    ManagedObjectType resp = {entry1, entry2};

    struct Extraction_Handlers : DbusBaseHandler
    {
        std::vector<handler_pair> LicenseExtractor;
        std::vector<handler_pair> AvailabilityExtractor;
        Extraction_Handlers()
        {
            LicenseExtractor.emplace_back("Name", uint32_t_node_mapper());
            handlers.emplace("xyz.openbmc_project.License.Entry.LicenseEntry",
                             make_extractor(LicenseExtractor));

            
        }
    };
    Extraction_Handlers extraction_handlers;
    DbusTreeParser parser(resp, extraction_handlers);
    bool errorfound = false;
    parser.on_error_message(
              [&](auto&& ) {
        errorfound = true;
    }).on_success([&](auto&& ) {
          
      }).parse();
    ASSERT_EQ(errorfound, true);
}
} // namespace
} // namespace redfish
