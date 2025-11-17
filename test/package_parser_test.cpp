// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "package_parser.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <ios>
#include <optional>
#include <ostream>
#include <sstream>
#include <utility>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

namespace
{
constexpr std::array<uint8_t, 16> packageUuid = {
    0xF0, 0x18, 0x87, 0x8c, 0xcb, 0x7d, 0x49, 0x43,
    0x98, 0x00, 0xa0, 0x2f, 0x05, 0x9a, 0xca, 0x02};

std::vector<uint8_t> buildDescriptorBytes(desc::Types type,
                                          const std::vector<uint8_t>& payload)
{
    std::vector<uint8_t> bytes;
    bytes.reserve(4 + payload.size());

    const uint16_t typeValue = static_cast<uint16_t>(type);
    bytes.push_back(static_cast<uint8_t>(typeValue & 0xFF));
    bytes.push_back(static_cast<uint8_t>((typeValue >> 8) & 0xFF));

    const uint16_t length = static_cast<uint16_t>(payload.size());
    bytes.push_back(static_cast<uint8_t>(length & 0xFF));
    bytes.push_back(static_cast<uint8_t>((length >> 8) & 0xFF));

    bytes.insert(bytes.end(), payload.begin(), payload.end());
    return bytes;
}

void appendLe16(std::vector<uint8_t>& buffer, uint16_t value)
{
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
}

void appendLe32(std::vector<uint8_t>& buffer, uint32_t value)
{
    appendLe16(buffer, static_cast<uint16_t>(value & 0xFFFF));
    appendLe16(buffer, static_cast<uint16_t>((value >> 16) & 0xFFFF));
}

std::vector<uint8_t> buildHeaderBytes(
    const std::vector<std::vector<uint8_t>>& descriptorBlobs,
    const std::vector<uint8_t>& componentBitmap,
    const std::vector<ImageInfo>& imageInfos)
{
    std::vector<uint8_t> bytes(packageUuid.begin(), packageUuid.end());
    bytes.resize(32, 0x00);

    appendLe16(bytes, static_cast<uint16_t>(
                          componentBitmap.size() * static_cast<size_t>(8)));
    bytes.push_back(0x00); // package version string type
    bytes.push_back(0x00); // package version string length

    bytes.push_back(1);    // idRecordCount

    const size_t recordStart = bytes.size();
    appendLe16(bytes, 0); // placeholder for record length
    bytes.push_back(static_cast<uint8_t>(descriptorBlobs.size()));
    bytes.insert(bytes.end(), 5, 0x00);
    bytes.push_back(0x00);     // componentImageVersionStringLength
    appendLe16(bytes, 0x0000); // firmwareDevicePackageLength
    bytes.insert(bytes.end(), componentBitmap.begin(), componentBitmap.end());

    for (const auto& blob : descriptorBlobs)
    {
        bytes.insert(bytes.end(), blob.begin(), blob.end());
    }

    const uint16_t recordLength =
        static_cast<uint16_t>(bytes.size() - recordStart);
    bytes[recordStart] = static_cast<uint8_t>(recordLength & 0xFF);
    bytes[recordStart + 1] = static_cast<uint8_t>((recordLength >> 8) & 0xFF);

    appendLe16(bytes, static_cast<uint16_t>(imageInfos.size()));

    for (const auto& image : imageInfos)
    {
        bytes.insert(bytes.end(), 12, 0x00);
        appendLe32(bytes, image.offset);
        appendLe32(bytes, image.length);
        bytes.push_back(0x00); // string type
        bytes.push_back(0x00); // version string length
    }

    appendLe32(bytes, 0x00000000); // crc placeholder

    const uint16_t headerSize = static_cast<uint16_t>(bytes.size());
    bytes[17] = static_cast<uint8_t>(headerSize & 0xFF);
    bytes[18] = static_cast<uint8_t>((headerSize >> 8) & 0xFF);

    return bytes;
}

struct DescriptorCase
{
    desc::Types type;
    std::vector<uint8_t> payload;
    std::function<void(const desc::Descriptor&)> validator;
};

const char* descriptorTypeName(desc::Types type)
{
    switch (type)
    {
        case desc::Types::PCI_VENDOR_ID:
            return desc::PciVendorId::name;
        case desc::Types::IANA_ENTERPRISE_ID:
            return desc::IanaEnterpriseId::name;
        case desc::Types::UUID:
            return desc::Uuid::name;
        case desc::Types::PNP_VENDOR_ID:
            return desc::PnpVendorId::name;
        case desc::Types::ACPI_VENDOR_ID:
            return desc::AcpiVendorId::name;
        case desc::Types::PCI_DEVICE_ID:
            return desc::PciDeviceId::name;
        case desc::Types::PCI_SUBSYSTEM_VENDOR_ID:
            return desc::PciSubsystemVendorId::name;
        case desc::Types::PCI_SUBSYSTEM_ID:
            return desc::PciSubsystemId::name;
        case desc::Types::PCI_REVISION_ID:
            return desc::PciRevisionId::name;
        case desc::Types::PNP_PRODUCT_ID:
            return desc::PnpProductId::name;
        case desc::Types::ACPI_PRODUCT_ID:
            return desc::AcpiProductId::name;
        case desc::Types::VENDOR_DEFINED:
            return desc::VendorDefined::name;
        default:
            return "UNKNOWN";
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
void PrintTo(const DescriptorCase& descCase, std::ostream* os)
{
    const uint16_t rawType = static_cast<uint16_t>(descCase.type);
    std::ostringstream encodedType;
    encodedType << "0x" << std::hex << std::uppercase << rawType;

    *os << "DescriptorCase{type=" << descriptorTypeName(descCase.type) << " ("
        << encodedType.str() << "), payload_len=" << descCase.payload.size()
        << "}";
}

const std::vector<DescriptorCase>& basicDescriptorCases()
{
    static const std::vector<DescriptorCase> cases = {
        {
            desc::Types::PCI_VENDOR_ID,
            {0x34, 0x12},
            [](const desc::Descriptor& descriptor) {
                ASSERT_TRUE(
                    std::holds_alternative<desc::PciVendorId>(descriptor.mId));
                const auto& value = std::get<desc::PciVendorId>(descriptor.mId);
                EXPECT_EQ(value.id, 0x1234);
            },
        },
        {
            desc::Types::IANA_ENTERPRISE_ID,
            {0x78, 0x56, 0x34, 0x12},
            [](const desc::Descriptor& descriptor) {
                ASSERT_TRUE(std::holds_alternative<desc::IanaEnterpriseId>(
                    descriptor.mId));
                const auto& value =
                    std::get<desc::IanaEnterpriseId>(descriptor.mId);
                EXPECT_EQ(value.id, 0x12345678);
            },
        },
        {
            desc::Types::UUID,
            {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
             0x0B, 0x0C, 0x0D, 0x0E, 0x0F},
            [](const desc::Descriptor& descriptor) {
                ASSERT_TRUE(std::holds_alternative<desc::Uuid>(descriptor.mId));
                const auto& value = std::get<desc::Uuid>(descriptor.mId);
                const std::array<uint8_t, 16> expected = {
                    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
                EXPECT_EQ(value.id, expected);
            },
        },
        {
            desc::Types::PNP_VENDOR_ID,
            {'O', 'B', 'M'},
            [](const desc::Descriptor& descriptor) {
                ASSERT_TRUE(
                    std::holds_alternative<desc::PnpVendorId>(descriptor.mId));
                const auto& value = std::get<desc::PnpVendorId>(descriptor.mId);
                const std::array<uint8_t, 3> expected = {'O', 'B', 'M'};
                EXPECT_EQ(value.id, expected);
            },
        },
        {
            desc::Types::ACPI_VENDOR_ID,
            {0xEF, 0xCD, 0xAB, 0x90},
            [](const desc::Descriptor& descriptor) {
                ASSERT_TRUE(
                    std::holds_alternative<desc::AcpiVendorId>(descriptor.mId));
                const auto& value =
                    std::get<desc::AcpiVendorId>(descriptor.mId);
                EXPECT_EQ(value.id, 0x90ABCDEF);
            },
        },
        {
            desc::Types::PCI_DEVICE_ID,
            {0x78, 0x56},
            [](const desc::Descriptor& descriptor) {
                ASSERT_TRUE(
                    std::holds_alternative<desc::PciDeviceId>(descriptor.mId));
                const auto& value = std::get<desc::PciDeviceId>(descriptor.mId);
                EXPECT_EQ(value.id, 0x5678);
            },
        },
        {
            desc::Types::PCI_SUBSYSTEM_VENDOR_ID,
            {0x44, 0x33},
            [](const desc::Descriptor& descriptor) {
                ASSERT_TRUE(std::holds_alternative<desc::PciSubsystemVendorId>(
                    descriptor.mId));
                const auto& value =
                    std::get<desc::PciSubsystemVendorId>(descriptor.mId);
                EXPECT_EQ(value.id, 0x3344);
            },
        },
        {
            desc::Types::PCI_SUBSYSTEM_ID,
            {0x22, 0x11},
            [](const desc::Descriptor& descriptor) {
                ASSERT_TRUE(std::holds_alternative<desc::PciSubsystemId>(
                    descriptor.mId));
                const auto& value =
                    std::get<desc::PciSubsystemId>(descriptor.mId);
                EXPECT_EQ(value.id, 0x1122);
            },
        },
        {
            desc::Types::PCI_REVISION_ID,
            {0xAA},
            [](const desc::Descriptor& descriptor) {
                ASSERT_TRUE(std::holds_alternative<desc::PciRevisionId>(
                    descriptor.mId));
                const auto& value =
                    std::get<desc::PciRevisionId>(descriptor.mId);
                EXPECT_EQ(value.id, 0xAA);
            },
        },
        {
            desc::Types::PNP_PRODUCT_ID,
            {0xDE, 0xBC, 0x9A, 0x78},
            [](const desc::Descriptor& descriptor) {
                ASSERT_TRUE(
                    std::holds_alternative<desc::PnpProductId>(descriptor.mId));
                const auto& value =
                    std::get<desc::PnpProductId>(descriptor.mId);
                EXPECT_EQ(value.id, 0x789ABCDE);
            },
        },
        {
            desc::Types::ACPI_PRODUCT_ID,
            {0x56, 0x34, 0x12, 0x00},
            [](const desc::Descriptor& descriptor) {
                ASSERT_TRUE(std::holds_alternative<desc::AcpiProductId>(
                    descriptor.mId));
                const auto& value =
                    std::get<desc::AcpiProductId>(descriptor.mId);
                EXPECT_EQ(value.id, 0x00123456);
            },
        },
    };
    return cases;
}
} // namespace
class DescriptorParsingTest : public ::testing::TestWithParam<DescriptorCase>
{};

TEST_P(DescriptorParsingTest, ParsesBasicDescriptors)
{
    const DescriptorCase& param = GetParam();
    const std::vector<uint8_t> bytes =
        buildDescriptorBytes(param.type, param.payload);

    std::optional<std::pair<desc::Descriptor, size_t>> parsed =
        desc::Descriptor::fromBytes(bytes);
    // unfortunately, clang-tidy doesn't see ASSERT on has_value
    // as an optional check :/
    // NOLINTBEGIN(bugprone-unchecked-optional-access)
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->second, param.payload.size() + 4);

    param.validator(parsed->first);
    // NOLINTEND(bugprone-unchecked-optional-access)
}

INSTANTIATE_TEST_SUITE_P(BasicDescriptors, DescriptorParsingTest,
                         ::testing::ValuesIn(basicDescriptorCases()));

TEST(DescNamespaceVendorDefinedTest, ParsesVendorDefinedDescriptors)
{
    const std::vector<uint8_t> payload = {
        0x01, // reserved byte consumed by the parser
        0x04, // string length
        'F',  'W', 'I', 'D', 0xAA, 0xBB, 0xCC, 0xDD};
    const std::vector<uint8_t> bytes =
        buildDescriptorBytes(desc::Types::VENDOR_DEFINED, payload);

    std::optional<std::pair<desc::Descriptor, size_t>> parsed =
        desc::Descriptor::fromBytes(bytes);
    // NOLINTBEGIN(bugprone-unchecked-optional-access)
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->second, payload.size() + 4);

    ASSERT_TRUE(std::holds_alternative<desc::VendorDefined>(parsed->first.mId));
    const desc::VendorDefined& vendorDefined =
        std::get<desc::VendorDefined>(parsed->first.mId);
    EXPECT_EQ(vendorDefined.title, "FWID");
    EXPECT_EQ(vendorDefined.data,
              std::vector<uint8_t>({0xAA, 0xBB, 0xCC, 0xDD}));
    // NOLINTEND(bugprone-unchecked-optional-access)
}

TEST(DescNamespaceVendorDefinedTest, RejectsIncompleteVendorDefinedStrings)
{
    const std::vector<uint8_t> payload = {
        0x00, // reserved byte
        0x04, // claims there are 4 bytes of name
        'F', 'W'};
    const std::vector<uint8_t> bytes =
        buildDescriptorBytes(desc::Types::VENDOR_DEFINED, payload);

    EXPECT_FALSE(desc::Descriptor::fromBytes(bytes));
}

TEST(DescNamespaceDescriptorErrors, RejectsTruncatedHeader)
{
    const std::array<uint8_t, 3> incompleteHeader = {0x00, 0x00, 0x00};
    std::optional<std::pair<desc::Descriptor, size_t>> res =
        desc::Descriptor::fromBytes(incompleteHeader);
    ASSERT_EQ(res, std::nullopt);
}

TEST(DescNamespaceDescriptorErrors, RejectsUnknownDescriptorType)
{
    const std::vector<uint8_t> bytes = {0xEF, 0xBE, 0x00, 0x00};
    std::optional<std::pair<desc::Descriptor, size_t>> res =
        desc::Descriptor::fromBytes(bytes);
    ASSERT_EQ(res, std::nullopt);
}

TEST(DescNamespaceDescriptorErrors, RejectsTruncatedPayloads)
{
    const std::vector<uint8_t> payload = {0xAA};
    const std::vector<uint8_t> bytes =
        buildDescriptorBytes(desc::Types::PCI_DEVICE_ID, payload);

    EXPECT_FALSE(desc::Descriptor::fromBytes(bytes));
}

TEST(PackageParserParseHeaderTest,
     ProcessBytesSucceedsWhenComponentMatchesRegisteredCallback)
{
    PackageParser parser(ParserVersion::VERSION_1_0_0);
    std::vector<desc::Descriptor> descriptors;
    descriptors.emplace_back(desc::PciVendorId{0x1234});
    parser.registerComponentRoute(
        [](const std::error_code&, std::span<const uint8_t>) {},
        std::move(descriptors));

    const std::vector<uint8_t> descriptorBlob =
        buildDescriptorBytes(desc::Types::PCI_VENDOR_ID, {0x34, 0x12});
    const std::vector<uint8_t> componentBitmap = {0x01};
    const std::vector<ImageInfo> imageInfos = {ImageInfo{0x40, 0x20}};
    const std::vector<uint8_t> header =
        buildHeaderBytes({descriptorBlob}, componentBitmap, imageInfos);

    EXPECT_TRUE(parser.processBytes(header));
    EXPECT_EQ(parser.getState(), PackageParser::state::PARSING_OUT_COMPONENTS);
}

TEST(PackageParserParseHeaderTest, ProcessBytesFailsWhenImageMissingDescriptor)
{
    PackageParser parser(ParserVersion::VERSION_1_0_0);
    std::vector<desc::Descriptor> descriptors;
    descriptors.emplace_back(desc::PciVendorId{0x1234});
    parser.registerComponentRoute(
        [](const std::error_code&, std::span<const uint8_t>) {},
        std::move(descriptors));

    const std::vector<uint8_t> descriptorBlob =
        buildDescriptorBytes(desc::Types::PCI_VENDOR_ID, {0x34, 0x12});
    const std::vector<uint8_t> componentBitmap = {0x03};
    const std::vector<ImageInfo> imageInfos = {ImageInfo{0x40, 0x20},
                                               ImageInfo{0x80, 0x10}};
    const std::vector<uint8_t> header =
        buildHeaderBytes({descriptorBlob}, componentBitmap, imageInfos);

    EXPECT_FALSE(parser.processBytes(header));
}

TEST(PackageParserParseHeaderTest,
     ProcessBytesFailsWhenNoRegisteredComponentsMatch)
{
    PackageParser parser(ParserVersion::VERSION_1_0_0);
    const std::vector<uint8_t> descriptorBlob =
        buildDescriptorBytes(desc::Types::PCI_VENDOR_ID, {0x34, 0x12});
    const std::vector<uint8_t> componentBitmap = {0x01};
    const std::vector<ImageInfo> imageInfos = {ImageInfo{0x40, 0x20}};
    const std::vector<uint8_t> header =
        buildHeaderBytes({descriptorBlob}, componentBitmap, imageInfos);

    EXPECT_FALSE(parser.processBytes(header));
}

TEST(PackageParserStateMachineTest, ProcessesChunksThroughHeaderStates)
{
    PackageParser parser(ParserVersion::VERSION_1_0_0);
    std::vector<desc::Descriptor> descriptors;
    descriptors.emplace_back(desc::PciVendorId{0x1234});
    parser.registerComponentRoute(
        [](const std::error_code&, std::span<const uint8_t>) {},
        std::move(descriptors));

    const std::vector<uint8_t> descriptorBlob =
        buildDescriptorBytes(desc::Types::PCI_VENDOR_ID, {0x34, 0x12});
    const std::vector<uint8_t> componentBitmap = {0x01};
    const std::vector<ImageInfo> imageInfos = {ImageInfo{0x40, 0x20}};
    const std::vector<uint8_t> header =
        buildHeaderBytes({descriptorBlob}, componentBitmap, imageInfos);

    std::vector<uint8_t> buffer;
    buffer.insert(buffer.end(), packageUuid.begin(),
                  packageUuid.begin() + packageUuid.size() - 2);
    buffer.push_back(packageUuid.back());
    EXPECT_TRUE(parser.processBytes(buffer));
    EXPECT_EQ(parser.getState(), PackageParser::state::WAITING_FOR_LENGTH);

    std::vector<uint8_t> headerLengthChunk(
        header.begin() + packageUuid.size(),
        header.begin() + packageUuid.size() + 3);
    EXPECT_TRUE(parser.processBytes(headerLengthChunk));
    EXPECT_EQ(parser.getState(), PackageParser::state::WAITING_FOR_HEADER);

    std::vector<uint8_t> remainder(header.begin() + packageUuid.size() + 3,
                                   header.end());
    EXPECT_TRUE(parser.processBytes(remainder));
    EXPECT_EQ(parser.getState(), PackageParser::state::PARSING_OUT_COMPONENTS);
}
