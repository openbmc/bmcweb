#include "utils/dbus_tree_parser.hpp"

#include <string>

#include <gtest/gtest.h> // IWYU pragma: keep

// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
// IWYU pragma: no_include "gtest/gtest_pred_impl.h"
using namespace dbus::utility;
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
    auto j = R"({"Type":"OriginatorTypes::Client"})"_json;
    OriginatorTypes mytype{};
    from_json(j["Type"], mytype);
    DBusPropertiesMap licenceEntry = {
        DBusPropertiesMap::value_type{"Name", DbusVariantType("some name")},
        {"AuthDeviceNumber", uint32_t(32)},
        {"ExpirationTime", uint64_t(3434324234)},
        {"SampleVec",
         AssociationsValType{{"he", "ll", "o"}, {"hi", "by", "e"}}},
        {"SampleBool", true},
        {"SerialNumber", "3434324234"}};
    DBusPropertiesMap availabilityEntry = {
        DBusPropertiesMap::value_type{
            "Available", DbusVariantType(std::vector<uint32_t>{1, 2})},
        DBusPropertiesMap::value_type{
            "New Entry", DbusVariantType(std::vector<uint32_t>{1, 2})}};
    DBusPropertiesMap logEntry = {DBusPropertiesMap::value_type{
        "OriginatorType",
        DbusVariantType(
            "xyz.openbmc_project.Common.OriginatedBy.OriginatorTypes.Internal")}};
    DBusPropertiesMap cableEntry = {
        DBusPropertiesMap::value_type{
            "CableTypeDescription",
            DbusVariantType(std::string("Super Cable"))},
        {"Length", DbusVariantType(3.45)}};
    return ManagedObjectType::value_type{
        sdbusplus::message::object_path(path),
        {{"xyz.openbmc_project.License.Entry.LicenseEntry", licenceEntry},
         {"xyz.openbmc_project.State.Decorator.Availability",
          availabilityEntry},
         {"xyz.openbmc_project.Common.OriginatedBy", logEntry},
         {"xyz.openbmc_project.Inventory.Item.Cable", cableEntry},
         {"xyz.openbmc_project.State.Decorator.Availability2",
          availabilityEntry}}};
}

TEST(TestParserPositive, PositiveCaseWithSuccess)
{
    auto entry1 = makeEntries("/xyz/openbmc_project/license/entry1/");
    auto entry2 = makeEntries("/xyz/openbmc_project/license/entry2/");
    ManagedObjectType resp = {entry1, entry2};

    struct ExtractionHandlers : DbusBaseMapper
    {
        ExtractionHandlers()
        {
            addInterfaceHandler(
                "xyz.openbmc_project.License.Entry.LicenseEntry", "Name",
                mapToKey<std::string>("Name"));
            addInterfaceHandler(
                "xyz.openbmc_project.License.Entry.LicenseEntry",
                "AuthDeviceNumber",
                mapToKeyOrError<uint32_t>("AuthDeviceNumber"));
            addInterfaceHandler(
                "xyz.openbmc_project.License.Entry.LicenseEntry",
                "SerialNumber",
                mapToKey<std::string>(
                    "SomeKey/New Key",
                    [](auto s, MapperResult& mr, MetaData& md) {
                if (mr != MapperResult::Ok)
                {
                    redfish::messages::setInternalError(md.res);
                    return std::string();
                }

                return s;
                    }));
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
    DbusTreeParser parser(extractionHandlers, true);
    bool errorfound = false;
    nlohmann::json result;
    parser.parse(
        resp, [&errorfound, &result](DbusParserStatus status, auto&& summary) {
            if (status == DbusParserStatus::Failed)
            {
                errorfound = true;
                return;
            }
            result = summary;
        });
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
    "SomeKey": {
        "New Key": "3434324234"
    }
})"_json;
    ASSERT_EQ(expected, result);
    // std::cout << result.dump(4);
}

TEST(TestParserNegative, ParsetWithErrorMessage)
{
    auto entry1 = makeEntries("/xyz/openbmc_project/license/entry1/");
    auto entry2 = makeEntries("/xyz/openbmc_project/license/entry2/");
    ManagedObjectType resp = {entry1, entry2};

    struct ExtractionHandlers : DbusBaseMapper
    {
        ExtractionHandlers()
        {
            addInterfaceHandler(
                "xyz.openbmc_project.License.Entry.LicenseEntry", "Name",
                mapToKey<uint32_t>("Name"));
        }
    };
    ExtractionHandlers extractionHandlers;
    DbusTreeParser parser(extractionHandlers);
    bool errorfound = false;
    parser.parse(resp, [&errorfound](DbusParserStatus status, auto&&) {
        errorfound = (status == DbusParserStatus::Failed);
    });
    ASSERT_EQ(errorfound, true);
}
TEST(TestMapperAddMap, TestAddMapApi)
{
    auto entry1 = makeEntries("/xyz/openbmc_project/license/entry1/");
    auto entry2 = makeEntries("/xyz/openbmc_project/license/entry2/");
    ManagedObjectType resp = {entry1};

    DbusBaseMapper mapper;

    mapper
        .addMap("CableTypeDescription",
                mapToKeyOrError<std::string>("CableType"))
        .addMap("Length", mapToKeyOrError<double, double>("LengthMeters",
                                                          [](auto length) {
        if (std::isfinite(length) && std::isnan(length))
        {
            return std::optional<double>();
        }
        return std::optional<double>(length);
                }));

    auto propMap = resp[0].second[3].second;
    DbusTreeParser parser(mapper, true);
    nlohmann::json result;
    parser.parse(propMap, [&result](DbusParserStatus /*unused*/,
                                    auto&& target) { result = target; });
    ASSERT_EQ(result, R"({
    "CableType": "Super Cable",
    "LengthMeters": 3.45
})"_json);
}
} // namespace
} // namespace redfish
