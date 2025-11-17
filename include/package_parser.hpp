#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <format>
#include <functional>
#include <iostream>
#include <optional>
#include <span>
#include <string_view>
#include <system_error>
#include <variant>
#include <vector>

// we need to be able to construct a Descriptor from something strongly typed
// we need to be able to construct a descriptor from a byte stream
namespace desc
{

enum class Types : uint16_t
{
    PCI_VENDOR_ID = 0,
    IANA_ENTERPRISE_ID = 1,
    UUID = 2,
    PNP_VENDOR_ID = 3,
    ACPI_VENDOR_ID = 4,
    PCI_DEVICE_ID = 0x0100,
    PCI_SUBSYSTEM_VENDOR_ID = 0x0101,
    PCI_SUBSYSTEM_ID = 0x0102,
    PCI_REVISION_ID = 0x0103,
    PNP_PRODUCT_ID = 0x0104,
    ACPI_PRODUCT_ID = 0x0105,
    VENDOR_DEFINED = 0xFFFF,
};

struct PciVendorId
{
    uint16_t id;
    static std::optional<std::pair<PciVendorId, size_t>> fromBytes(
        std::span<const uint8_t> bytes);
    auto operator<=>(const PciVendorId&) const = default;
    static constexpr const char* name = "PciVendorId";
};

struct IanaEnterpriseId
{
    uint32_t id;
    static std::optional<std::pair<IanaEnterpriseId, size_t>> fromBytes(
        std::span<const uint8_t> bytes);
    auto operator<=>(const IanaEnterpriseId&) const = default;
    static constexpr const char* name = "IanaEnterpriseId";
};

struct Uuid
{
    std::array<uint8_t, 16> id;
    static std::optional<std::pair<Uuid, size_t>> fromBytes(
        std::span<const uint8_t> bytes);
    auto operator<=>(const Uuid&) const = default;
    static constexpr const char* name = "Uuid";
};

struct PnpVendorId
{
    std::array<uint8_t, 3> id;
    static std::optional<std::pair<PnpVendorId, size_t>> fromBytes(
        std::span<const uint8_t> bytes);
    auto operator<=>(const PnpVendorId&) const = default;
    static constexpr const char* name = "PnpVendorId";
};

struct AcpiVendorId
{
    // TODO: revisit if this is correct: should this be ascii?
    uint32_t id;
    static std::optional<std::pair<AcpiVendorId, size_t>> fromBytes(
        std::span<const uint8_t> bytes);
    auto operator<=>(const AcpiVendorId&) const = default;
    static constexpr const char* name = "AcpiVendorId";
};

struct PciDeviceId
{
    uint16_t id;
    static std::optional<std::pair<PciDeviceId, size_t>> fromBytes(
        std::span<const uint8_t> bytes);
    auto operator<=>(const PciDeviceId&) const = default;
    static constexpr const char* name = "PciDeviceId";
};

struct PciSubsystemVendorId
{
    uint16_t id;
    static std::optional<std::pair<PciSubsystemVendorId, size_t>> fromBytes(
        std::span<const uint8_t> bytes);
    auto operator<=>(const PciSubsystemVendorId&) const = default;
    static constexpr const char* name = "PciSubsystemVendorId";
};

struct PciSubsystemId
{
    uint16_t id;
    static std::optional<std::pair<PciSubsystemId, size_t>> fromBytes(
        std::span<const uint8_t> bytes);
    auto operator<=>(const PciSubsystemId&) const = default;
    static constexpr const char* name = "PciSubsystemId";
};

struct PciRevisionId
{
    uint8_t id;
    static std::optional<std::pair<PciRevisionId, size_t>> fromBytes(
        std::span<const uint8_t> bytes);
    auto operator<=>(const PciRevisionId&) const = default;
    static constexpr const char* name = "PciRevisionId";
};

struct PnpProductId
{
    uint32_t id;
    static std::optional<std::pair<PnpProductId, size_t>> fromBytes(
        std::span<const uint8_t> bytes);
    auto operator<=>(const PnpProductId&) const = default;
    static constexpr const char* name = "PnpProductId";
};

struct AcpiProductId
{
    uint32_t id;
    static std::optional<std::pair<AcpiProductId, size_t>> fromBytes(
        std::span<const uint8_t> bytes);
    auto operator<=>(const AcpiProductId&) const = default;
    static constexpr const char* name = "AcpiProductId";
};

struct VendorDefined
{
    std::string title;
    std::vector<uint8_t> data;
    static std::optional<std::pair<VendorDefined, size_t>> fromBytes(
        std::span<const uint8_t> bytes, uint16_t len);
    auto operator<=>(const VendorDefined&) const = default;
    static constexpr const char* name = "Vendor Defined";
};

struct Descriptor
{
    Descriptor(const PciVendorId& id) : mId{id} {}
    Descriptor(const IanaEnterpriseId& id) : mId{id} {}
    Descriptor(const PnpVendorId& id) : mId{id} {}
    Descriptor(const AcpiVendorId& id) : mId{id} {}
    Descriptor(const PciDeviceId& id) : mId{id} {}
    Descriptor(const PciSubsystemVendorId& id) : mId{id} {}
    Descriptor(const PciSubsystemId& id) : mId{id} {}
    Descriptor(const PciRevisionId& id) : mId{id} {}
    Descriptor(const PnpProductId& id) : mId{id} {}
    Descriptor(const AcpiProductId& id) : mId{id} {}
    Descriptor(const VendorDefined& id) : mId{id} {}
    Descriptor(const Uuid& id) : mId{id} {}

    static std::optional<std::pair<Descriptor, size_t>> fromBytes(
        std::span<const uint8_t> bytes)
    {
        if (bytes.size() < 4)
        {
            return std::nullopt;
        }

        uint16_t type = bytes[0];
        type |= static_cast<uint16_t>(bytes[1] << 8);

        uint16_t length = bytes[2];
        length |= static_cast<uint16_t>(bytes[3] << 8);

        size_t bytesParsed = 4;

        // PLEASE GOD GIVE ME COMPILE TIME REFLECTION
        bytes = bytes.subspan(bytesParsed);
        switch (static_cast<Types>(type))
        {
            case Types::PCI_VENDOR_ID:
                return parseSubComponent<PciVendorId>(bytes);
            case Types::IANA_ENTERPRISE_ID:
                return parseSubComponent<IanaEnterpriseId>(bytes);
            case Types::UUID:
                return parseSubComponent<Uuid>(bytes);
            case Types::PNP_VENDOR_ID:
                return parseSubComponent<PnpVendorId>(bytes);
            case Types::ACPI_VENDOR_ID:
                return parseSubComponent<AcpiVendorId>(bytes);
            case Types::PCI_DEVICE_ID:
                return parseSubComponent<PciDeviceId>(bytes);
            case Types::PCI_SUBSYSTEM_VENDOR_ID:
                return parseSubComponent<PciSubsystemVendorId>(bytes);
            case Types::PCI_SUBSYSTEM_ID:
                return parseSubComponent<PciSubsystemId>(bytes);
            case Types::PCI_REVISION_ID:
                return parseSubComponent<PciRevisionId>(bytes);
            case Types::PNP_PRODUCT_ID:
                return parseSubComponent<PnpProductId>(bytes);
            case Types::ACPI_PRODUCT_ID:
                return parseSubComponent<AcpiProductId>(bytes);
            case Types::VENDOR_DEFINED:
            {
                std::optional<std::pair<VendorDefined, size_t>> res =
                    VendorDefined::fromBytes(bytes, length);
                if (!res)
                {
                    return std::nullopt;
                }
                return std::make_pair(res->first, res->second + 4);
            }
            default:
                std::cerr << std::format("unhandled descriptor type 0x{:x}\n",
                                         type);
                return std::nullopt;
        }
    }

    auto operator<=>(const Descriptor& other) const = default;

    std::variant<PciVendorId, IanaEnterpriseId, Uuid, PnpVendorId, AcpiVendorId,
                 PciDeviceId, PciSubsystemVendorId, PciSubsystemId,
                 PciRevisionId, PnpProductId, AcpiProductId, VendorDefined>
        mId;

  private:
    template <typename T>
    static std::optional<std::pair<Descriptor, size_t>> parseSubComponent(
        std::span<const uint8_t> bytes)
    {
        std::optional<std::pair<T, size_t>> res = T::fromBytes(bytes);
        if (!res)
        {
            return std::nullopt;
        }
        return std::make_pair(res->first, res->second + 4);
    }
};
} // namespace desc

enum class ParserVersion
{
    VERSION_1_0_0,
};

struct ImageInfo
{
    uint32_t offset;
    uint32_t length;
};

struct ImageInfoCaller
{
    ImageInfo info;
    std::function<void(const std::error_code&, std::span<const uint8_t>)> cb;
};

// the spec implies that we have to know all package descriptors
// that are supported ahead of time
// so we can have users register a callback to perform update

// general flow:
// 1. we get a callback with some bytes
// 2. we parse until we have a header, at which point we make a bunch
//       of ImageInfos
// 3. we continue to get called until we have hit a given image
// 4. once we get a given image, we call the callback registered with a given
//       set of descriptors
// general state machine:
// 1. Add bytes until header is complete
// 2. Once the header is complete, we can do parsing and identification of FD's
// 3.
struct Caller
{
    std::vector<desc::Descriptor> descriptors;
    std::function<void(const std::error_code&, std::span<const uint8_t>)> cb;
};

// TODO: better error reporting in general
class PackageParser
{
  public:
    PackageParser(ParserVersion version);
    void registerComponentRoute(
        const std::function<void(const std::error_code&,
                                 std::span<const uint8_t>)>& callback,
        std::vector<desc::Descriptor>&& descs);
    bool processBytes(std::span<const uint8_t> bytes);

    enum class state
    {
        WAITING_FOR_UUID,
        WAITING_FOR_LENGTH,
        WAITING_FOR_HEADER,
        PARSING_OUT_COMPONENTS,
        DONE,
    };

    state getState() const
    {
        return parse_state;
    }

  private:
    std::vector<uint8_t> headerBytes;
    std::vector<ImageInfoCaller> images;
    std::optional<size_t> updateStateMachine(std::span<const uint8_t> bytes);
    bool setupComponents();
    size_t handoutFirmwareImage(std::span<const uint8_t> bytes);
    size_t currentImage{};
    uint64_t bytesReceived{};
    uint16_t headerSize{};
    uint64_t totalBytesToReceive{};
    std::vector<Caller> registeredComponents;
    state parse_state{state::WAITING_FOR_UUID};
};
