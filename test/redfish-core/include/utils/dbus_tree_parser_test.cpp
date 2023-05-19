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
enum class OriginatorTypes
{
    Invalid,
    Client,
    Internal,
    SupportingService,
};
NLOHMANN_JSON_SERIALIZE_ENUM(OriginatorTypes,
                             {
                                 {OriginatorTypes::Invalid, "Invalid"},
                                 {OriginatorTypes::Client, "Client"},
                                 {OriginatorTypes::Internal, "Internal"},
                                 {OriginatorTypes::SupportingService,
                                  "SupportingService"},
                             });
inline OriginatorTypes
    mapDbusOriginatorTypeToRedfish(const std::string& originatorType)
{
    if (originatorType ==
        "xyz.openbmc_project.Common.OriginatedBy.OriginatorTypes.Client")
    {
        return OriginatorTypes::Client;
    }
    if (originatorType ==
        "xyz.openbmc_project.Common.OriginatedBy.OriginatorTypes.Internal")
    {
        return OriginatorTypes::Internal;
    }
    if (originatorType ==
        "xyz.openbmc_project.Common.OriginatedBy.OriginatorTypes.SupportingService")
    {
        return OriginatorTypes::SupportingService;
    }
    return OriginatorTypes::Invalid;
}
auto makeEntries(auto path)
{
    DU::DBusPropertiesMap licenceEntry = {
        DU::DBusPropertiesMap::value_type{"Name",
                                          DU::DbusVariantType("some name")},
        {"AuthDeviceNumber", uint32_t(32)},
        {"ExpirationTime", uint64_t(3434324234)},
        {"SampleVec",
         AssociationsValType{{"he", "ll", "o"}, {"hi", "by", "e"}}},
        {"SampleBool", true},
        {"SerialNumber", "3434324234"}};
    DU::DBusPropertiesMap availabilityEntry = {
        DU::DBusPropertiesMap::value_type{
            "Available", DU::DbusVariantType(std::vector<uint32_t>{1, 2})},
        DU::DBusPropertiesMap::value_type{
            "New Entry", DU::DbusVariantType(std::vector<uint32_t>{1, 2})}};
    DU::DBusPropertiesMap log_entry = {DU::DBusPropertiesMap::value_type{
        "OriginatorType",
        DU::DbusVariantType(
            "xyz.openbmc_project.Common.OriginatedBy.OriginatorTypes.Internal")}};

    return DU::ManagedObjectType::value_type{
        sdbusplus::message::object_path(path),
        {{"xyz.openbmc_project.License.Entry.LicenseEntry", licenceEntry},
         {"xyz.openbmc_project.State.Decorator.Availability",
          availabilityEntry},
         {"xyz.openbmc_project.Common.OriginatedBy", log_entry},
         {"xyz.openbmc_project.State.Decorator.Availability2",
          availabilityEntry}}};
}
TEST(TestParserPositive, PositiveCaseWithSuccess)
{
    auto entry1 = makeEntries("/xyz/openbmc_project/license/entry1/");
    auto entry2 = makeEntries("/xyz/openbmc_project/license/entry2/");
    DU::ManagedObjectType resp = {entry1, entry2};

    struct ExtractionHandlers : DbusBaseHandler
    {
        ExtractionHandlers()
        {
            addInterfaceHandler(
                "xyz.openbmc_project.License.Entry.LicenseEntry", "Name",
                mapToKey<std::string>("Name"));
            addInterfaceHandler(
                "xyz.openbmc_project.License.Entry.LicenseEntry",
                "AuthDeviceNumber", mapToKey<uint32_t>("AuthDeviceNumber"));
            addInterfaceHandler(
                "xyz.openbmc_project.License.Entry.LicenseEntry",
                "SerialNumber", mapToKey<std::string>("SerialNumber"));
            addInterfaceHandler(
                "xyz.openbmc_project.License.Entry.LicenseEntry", "SampleVec",
                mapToKey<AssociationsValType>("SampleVec"));

            addInterfaceHandler(
                "xyz.openbmc_project.State.Decorator.Availability", "Available",
                mapToHandler<std::vector<uint32_t>>(
                    [](const sdbusplus::message::object_path& path,
                       auto&& val) {
                nlohmann::json j;
                if (path == "/xyz/openbmc_project/license/entry1/")
                {
                    j["Availability"] = val;
                    return j;
                }
                j["Availability2"] = "Converted Value";
                return j;
                }));
            addInterfaceHandler(
                "xyz.openbmc_project.Common.OriginatedBy", "OriginatorType",
                mapToEnumKey<OriginatorTypes>("OriginatorType",
                                              mapDbusOriginatorTypeToRedfish));
        }
    };
    ExtractionHandlers extractionHandlers;
    DbusTreeParser parser(resp, extractionHandlers, true);
    bool errorfound = false;
    nlohmann::json result;
    parser
        .onSuccess(
            [&errorfound, &result](DbusParserStatus status, auto&& summary) {
        if (status == DbusParserStatus::Failed)
        {
            errorfound = true;
            return;
        }
        result = summary;
        })
        .parse();
    ASSERT_NE(errorfound, true);
    auto expected = R"({
    "AuthDeviceNumber": 32,
    "Availability": [
        1,
        2
    ],
    "Availability2": "Converted Value",
    "Name": "some name",
    "OriginatorType": "Internal",
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
})"_json;
    ASSERT_EQ(expected, result);
    // std::cout << result.dump(4);
}

TEST(TestParserNegative, ParsetWithErrorMessage)
{
    auto entry1 = makeEntries("/xyz/openbmc_project/license/entry1/");
    auto entry2 = makeEntries("/xyz/openbmc_project/license/entry2/");
    DU::ManagedObjectType resp = {entry1, entry2};

    struct ExtractionHandlers : DbusBaseHandler
    {
        ExtractionHandlers()
        {
            addInterfaceHandler(
                "xyz.openbmc_project.License.Entry.LicenseEntry", "Name",
                mapToKey<uint32_t>("Name"));
        }
    };
    ExtractionHandlers extractionHandlers;
    DbusTreeParser parser(resp, extractionHandlers);
    bool errorfound = false;
    parser
        .onSuccess([&errorfound](DbusParserStatus status, auto&&) {
            errorfound = (status == DbusParserStatus::Failed);
        })
        .parse();
    ASSERT_EQ(errorfound, true);
}
} // namespace
} // namespace redfish
