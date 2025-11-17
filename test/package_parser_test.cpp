// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "package_parser.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

namespace
{

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

struct DescriptorCase
{
    desc::Types type;
    std::vector<uint8_t> payload;
    std::function<void(const desc::Descriptor&)> validator;
};

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
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->second, param.payload.size() + 4);

    param.validator(parsed->first);
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
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->second, payload.size() + 4);

    ASSERT_TRUE(std::holds_alternative<desc::VendorDefined>(parsed->first.mId));
    const desc::VendorDefined& vendorDefined =
        std::get<desc::VendorDefined>(parsed->first.mId);
    EXPECT_EQ(vendorDefined.title, "FWID");
    EXPECT_EQ(vendorDefined.data,
              std::vector<uint8_t>({0xAA, 0xBB, 0xCC, 0xDD}));
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
    EXPECT_EQ(res, std::nullopt);
}

TEST(DescNamespaceDescriptorErrors, RejectsTruncatedPayloads)
{
    const std::vector<uint8_t> payload = {0xAA};
    const std::vector<uint8_t> bytes =
        buildDescriptorBytes(desc::Types::PCI_DEVICE_ID, payload);

    EXPECT_FALSE(desc::Descriptor::fromBytes(bytes));
}
