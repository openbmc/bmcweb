#include "package_parser.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <functional>
#include <iostream>
#include <iterator>
#include <optional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

struct IncrementalParser
{
    IncrementalParser(std::span<const uint8_t> inData) : data{inData} {}

    template <typename T>
    std::optional<std::type_identity_t<T>> get()
    {
        static_assert(std::is_trivially_copyable_v<T>);
        if (sizeof(T) > remainingBytes())
        {
            return std::nullopt;
        }
        // TODO: move away from memcpy
        T t;
        std::memcpy(&t, data.data(), sizeof(t));
        data = data.subspan(sizeof(t));
        return t;
    }

    template <typename T, size_t N>
    bool get(std::array<T, N>& arr)
    {
        if (N > remainingBytes())
        {
            return false;
        }
        std::copy_n(data.begin(), N, arr.begin());
        data = data.subspan(N);
        return true;
    }

    bool get(std::string& str, uint16_t length)
    {
        if (length > remainingBytes())
        {
            return false;
        }
        str = {std::bit_cast<const char*>(data.data()), length};
        data = data.subspan(length);
        return true;
    }

    bool get(std::vector<uint8_t>& v, size_t n)
    {
        if (n > remainingBytes())
        {
            return false;
        }
        std::copy_n(data.begin(), n, std::back_inserter(v));
        data = data.subspan(n);
        return true;
    }

    bool consume(size_t bytes)
    {
        if (bytes > remainingBytes())
        {
            return false;
        }
        data = data.subspan(bytes);
        return true;
    }

    size_t remainingBytes() const
    {
        return data.size();
    }

    std::span<const uint8_t> data;
};

template <>
std::optional<std::type_identity_t<desc::Descriptor>>
    IncrementalParser::get<desc::Descriptor>()
{
    std::optional<std::pair<desc::Descriptor, size_t>> res =
        desc::Descriptor::fromBytes(data);
    if (!res)
    {
        return std::nullopt;
    }
    data = data.subspan(res->second);
    return res->first;
}

namespace desc
{

template <typename T>
std::optional<std::pair<std::type_identity_t<T>, size_t>> parseCommon(
    std::span<const uint8_t> bytes)
{
    IncrementalParser p(bytes);
    std::optional<decltype(T::id)> val = p.get<decltype(T::id)>();
    if (!val)
    {
        return std::nullopt;
    }
    return std::make_pair(T{*val}, sizeof(T::id));
}

std::optional<std::pair<PciVendorId, size_t>> PciVendorId::fromBytes(
    std::span<const uint8_t> bytes)
{
    return parseCommon<PciVendorId>(bytes);
}

std::optional<std::pair<IanaEnterpriseId, size_t>> IanaEnterpriseId::fromBytes(
    std::span<const uint8_t> bytes)
{
    return parseCommon<IanaEnterpriseId>(bytes);
}

std::optional<std::pair<Uuid, size_t>> Uuid::fromBytes(
    std::span<const uint8_t> bytes)
{
    IncrementalParser p{bytes};
    Uuid id{};
    bool success = p.get(id.id);
    if (!success)
    {
        return std::nullopt;
    }
    return std::make_pair(id, id.id.size());
}

std::optional<std::pair<PnpVendorId, size_t>> PnpVendorId::fromBytes(
    std::span<const uint8_t> bytes)
{
    IncrementalParser p{bytes};
    PnpVendorId id{};
    bool success = p.get(id.id);
    if (!success)
    {
        return std::nullopt;
    }
    return std::make_pair(id, id.id.size());
}

std::optional<std::pair<AcpiVendorId, size_t>> AcpiVendorId::fromBytes(
    std::span<const uint8_t> bytes)
{
    return parseCommon<AcpiVendorId>(bytes);
}

std::optional<std::pair<PciDeviceId, size_t>> PciDeviceId::fromBytes(
    std::span<const uint8_t> bytes)
{
    return parseCommon<PciDeviceId>(bytes);
}

std::optional<std::pair<PciSubsystemVendorId, size_t>>
    PciSubsystemVendorId::fromBytes(std::span<const uint8_t> bytes)
{
    return parseCommon<PciSubsystemVendorId>(bytes);
}

std::optional<std::pair<PciSubsystemId, size_t>> PciSubsystemId::fromBytes(
    std::span<const uint8_t> bytes)
{
    return parseCommon<PciSubsystemId>(bytes);
}

std::optional<std::pair<PciRevisionId, size_t>> PciRevisionId::fromBytes(
    std::span<const uint8_t> bytes)
{
    return parseCommon<PciRevisionId>(bytes);
}

std::optional<std::pair<PnpProductId, size_t>> PnpProductId::fromBytes(
    std::span<const uint8_t> bytes)
{
    return parseCommon<PnpProductId>(bytes);
}

std::optional<std::pair<AcpiProductId, size_t>> AcpiProductId::fromBytes(
    std::span<const uint8_t> bytes)
{
    return parseCommon<AcpiProductId>(bytes);
}

std::optional<std::pair<VendorDefined, size_t>> VendorDefined::fromBytes(
    std::span<const uint8_t> bytes, uint16_t length)
{
    IncrementalParser p({bytes.data(), length});
    bool success = p.consume(1);
    if (!success)
    {
        return std::nullopt;
    }
    std::optional<uint8_t> strLength = p.get<uint8_t>();
    if (!strLength)
    {
        return std::nullopt;
    }
    std::string idName;
    success = p.get(idName, *strLength);
    if (!success)
    {
        return std::nullopt;
    }

    std::vector<uint8_t> data;
    success = p.get(data, p.remainingBytes());
    if (!success)
    {
        return std::nullopt;
    }
    return std::make_pair(VendorDefined{std::move(idName), std::move(data)},
                          length);
}

} // namespace desc

PackageParser::PackageParser(ParserVersion version)
{
    if (version != ParserVersion::VERSION_1_0_0)
    {
        throw std::runtime_error("Only version 1.0.0 is supported");
    }
}

void PackageParser::registerComponentRoute(
    const std::function<void(const std::error_code&, std::span<const uint8_t>)>&
        callback,
    std::vector<desc::Descriptor>&& descs)
{
    std::vector<desc::Descriptor> d = std::move(descs);
    std::ranges::sort(d);
    registeredComponents.emplace_back(std::move(d), callback);
}

size_t PackageParser::handoutFirmwareImage(std::span<const uint8_t> bytes)
{
    if (bytes.empty())
    {
        return 0;
    }

    while (currentImage < images.size() &&
           images[currentImage].info.length == 0U)
    {
        ++currentImage;
    }

    if (currentImage >= images.size())
    {
        // No more registered images. Consume the remainder so the state
        // machine can continue to advance.
        return bytes.size();
    }

    const ImageInfoCaller& image = images[currentImage];

    size_t leftImageBoundary = image.info.offset;
    size_t rightImageBoundary = image.info.offset + image.info.length;

    size_t leftBufferBoundary = bytesReceived;
    size_t rightBufferBoundary = bytesReceived + bytes.size();

    if (rightBufferBoundary <= leftImageBoundary)
    {
        // Entire buffer precedes the image; drop it so caller can feed more.
        return bytes.size();
    }

    if (leftBufferBoundary >= rightImageBoundary)
    {
        // Already past this image; move to the next one.
        ++currentImage;
        return 0;
    }

    size_t intersectionStart = std::max(leftBufferBoundary, leftImageBoundary);
    size_t offsetIntoBuffer = intersectionStart - leftBufferBoundary;

    if (offsetIntoBuffer != 0U)
    {
        // Tell the caller we've consumed the bytes leading up to the image.
        return offsetIntoBuffer;
    }

    size_t bytesRemainingInImage = rightImageBoundary - intersectionStart;
    size_t bytesToDeliver = std::min(bytesRemainingInImage, bytes.size());

    std::span<const uint8_t> slice = bytes.first(bytesToDeliver);
    image.cb(std::error_code{}, slice);

    if (bytesToDeliver == bytesRemainingInImage)
    {
        ++currentImage;
    }

    return bytesToDeliver;
}

std::optional<std::vector<PackageParser::DescriptorMatcher>>
    PackageParser::parseDescriptors(IncrementalParser& parser, uint8_t count,
                                    uint16_t bitMapLength)
{
    std::vector<DescriptorMatcher> matchers;
    for (uint8_t idCtr = 0; idCtr < count; ++idCtr)
    {
        size_t recordBeginningSize = parser.data.size();
        std::optional<uint16_t> recordLength = parser.get<uint16_t>();
        if (!recordLength)
        {
            return std::nullopt;
        }

        std::optional<uint8_t> descriptorCount = parser.get<uint8_t>();
        if (!descriptorCount)
        {
            return std::nullopt;
        }
        bool success = parser.consume(5);
        if (!success)
        {
            return std::nullopt;
        }
        std::optional<uint8_t> componentImageVersionStringLength =
            parser.get<uint8_t>();
        if (!componentImageVersionStringLength)
        {
            return std::nullopt;
        }
        std::optional<uint16_t> firmwareDevicePackageLength =
            parser.get<uint16_t>();
        if (!firmwareDevicePackageLength)
        {
            return std::nullopt;
        }

        DescriptorMatcher matcher;
        success = parser.get(matcher.components, bitMapLength);
        if (!success)
        {
            return std::nullopt;
        }
        success = parser.consume(*componentImageVersionStringLength);
        if (!success)
        {
            return std::nullopt;
        }

        // we're now looking at the recordDescriptors, parse those out
        for (uint8_t descriptorCounter = 0;
             descriptorCounter < *descriptorCount; ++descriptorCounter)
        {
            std::optional<desc::Descriptor> desc =
                parser.get<desc::Descriptor>();
            if (!desc)
            {
                std::cerr << "Failed to parse desc\n";
                return std::nullopt;
            }
            matcher.desc.push_back(std::move(*desc));
        }

        std::ranges::sort(matcher.desc);
        matchers.push_back(std::move(matcher));

        parser.consume(*firmwareDevicePackageLength);

        size_t recordEndingSize = parser.data.size();
        size_t totalSizeUsed = recordBeginningSize - recordEndingSize;
        if (totalSizeUsed != *recordLength)
        {
            // sanity check that we've consumed the correct number of bytes
            std::cerr << std::format(
                "Error: wrong byte count {} vs recordLength {}\n",
                totalSizeUsed, *recordLength);
            return std::nullopt;
        }
    }
    return matchers;
}

bool PackageParser::parseComponents(
    IncrementalParser& parser, const std::vector<DescriptorMatcher>& matchers,
    uint16_t imageCount)
{
    bool matchFound = false;
    for (uint16_t imageCtr = 0; imageCtr < imageCount; ++imageCtr)
    {
        // for each component, go lookup in the filters array which descriptor
        // set describes it and go look it up in the registered filter set this
        // is relatively expensive, but we only do it once we just need to parse
        // the offset and size of each component the component image information
        // is sized by a fix sized portion followed by a variable length string
        // with a fixed offset length.
        bool success = parser.consume(12);
        if (!success)
        {
            return false;
        }

        ImageInfo imageInfo{};
        std::optional<uint32_t> offset = parser.get<uint32_t>();
        if (!offset)
        {
            return false;
        }

        imageInfo.offset = *offset;
        std::optional<uint32_t> size = parser.get<uint32_t>();
        if (!size)
        {
            return false;
        }
        imageInfo.length = *size;

        totalBytesToReceive += imageInfo.length;

        // skip the string type - we aren't doing anything with it
        success = parser.consume(1);
        if (!success)
        {
            return false;
        }

        std::optional<uint8_t> componentVersionStringLength =
            parser.get<uint8_t>();
        if (!componentVersionStringLength)
        {
            return false;
        }
        success = parser.consume(*componentVersionStringLength);
        if (!success)
        {
            return false;
        }

        auto matcherIt = std::ranges::find_if(
            matchers, [imageCtr](const DescriptorMatcher& el) -> bool {
                uint16_t byteOffset = imageCtr / 8;
                uint16_t bitOffset = imageCtr % 8;
                if (byteOffset >= el.components.size())
                {
                    return false;
                }

                return el.components[byteOffset] & (1 << bitOffset);
            });

        if (matcherIt == matchers.end())
        {
            return false;
        }

        // now we can get the descriptor
        // so lets get the descriptor and match it up against the callback set
        // if none is found, thats okay, we'll just move on with life
        // for each caller, go through the package identifier set we just found
        // search through registeredComponents for a descriptor set that
        // includes the descriptor set that is applicable to this component
        auto registeredIt = std::ranges::find_if(
            registeredComponents, [matcherIt](const Caller& c) -> bool {
                return std::ranges::includes(c.descriptors, matcherIt->desc);
            });
        if (registeredIt == registeredComponents.end())
        {
            continue;
        }
        matchFound = true;
        images.emplace_back(imageInfo, registeredIt->cb);
    }

    return matchFound;
}

bool PackageParser::parseHeader()
{
    // we have a full header
    // go through and match device ID records to components
    // then go through the components and store off their offsets
    // the end of the package header information table
    // is given by 35 + the length of the package version string
    // the length of that string is given by byte 33, length 1
    IncrementalParser parser{headerBytes};
    bool success = parser.consume(32);
    if (!success)
    {
        return false;
    }
    std::optional<uint16_t> componentBitMapLength = parser.get<uint16_t>();
    if (!componentBitMapLength)
    {
        return false;
    }
    if ((*componentBitMapLength % 8) != 0)
    {
        throw std::runtime_error("invalid bitmap length");
    }
    uint16_t componentBitMapLengthBytes = *componentBitMapLength / 8;

    // skip the package version string type
    success = parser.consume(1);
    if (!success)
    {
        return false;
    }
    std::optional<uint8_t> strLength = parser.get<uint8_t>();
    if (!strLength)
    {
        return false;
    }
    success = parser.consume(*strLength);
    if (!success)
    {
        return false;
    }

    // first we need to go through the device identification area and collect
    // components and descriptors then for the bitfields we matched against,
    // coallate component images so we can stream them out later
    std::optional<uint8_t> idRecordCount = parser.get<uint8_t>();
    if (!idRecordCount)
    {
        return false;
    }

    std::optional<std::vector<DescriptorMatcher>> optMatchers =
        parseDescriptors(parser, *idRecordCount, componentBitMapLengthBytes);
    if (!optMatchers)
    {
        return false;
    }

    const std::vector<DescriptorMatcher>& matchers = *optMatchers;
    // we now have a parsed list of descriptors and what components they apply
    // to we are also now pointing to the beginning of the component image info
    // area thank god, this is significantly easier to parse
    std::optional<uint16_t> componentImageCount = parser.get<uint16_t>();
    if (!componentImageCount)
    {
        return false;
    }

    totalBytesToReceive = headerSize;
    success = parseComponents(parser, matchers, *componentImageCount);
    if (!success)
    {
        return false;
    }

    // we now have a list of components ordered by appearance in the image
    // with their start and size with respect to the beginning of the header
    // when we receive more bytes past the header, we just calculate where we
    // are in the list and send it along
    std::optional<uint32_t> crc = parser.get<uint32_t>();
    if (!crc)
    {
        return false;
    }

    // iterative parser should now be at the end of the header
    return parser.remainingBytes() == 0;
}

static constexpr std::array<uint8_t, 16> uuid = {
    0xF0, 0x18, 0x87, 0x8c, 0xcb, 0x7d, 0x49, 0x43,
    0x98, 0x00, 0xa0, 0x2f, 0x05, 0x9a, 0xca, 0x02};

std::optional<size_t> PackageParser::updateStateMachine(
    std::span<const uint8_t> bytes)
{
    if (parse_state == state::WAITING_FOR_UUID)
    {
        static size_t constexpr uuidOffset = 0;
        static size_t constexpr uuidLength = 16;
        if (bytes.size() + bytesReceived >= uuidOffset + uuidLength)
        {
            size_t bytesToParse = uuidLength - bytesReceived;
            std::copy_n(bytes.begin(), uuidLength,
                        std::back_inserter(headerBytes));
            if (!std::equal(uuid.begin(), uuid.end(), headerBytes.begin()))
            {
                std::cerr << "incorrect uuid\n";
                return std::nullopt;
            }
            std::cerr << "move to length check\n";
            parse_state = state::WAITING_FOR_LENGTH;
            return bytesToParse;
        }
        std::copy(bytes.begin(), bytes.end(), std::back_inserter(headerBytes));
        return bytes.size();
    }
    if (parse_state == state::WAITING_FOR_LENGTH)
    {
        static size_t constexpr headerLengthOffset = 17;
        static size_t constexpr headerLengthBytesNeeded = 19;
        if (bytes.size() + bytesReceived >=
            headerLengthOffset + sizeof(headerSize))
        {
            size_t bytesToParse = headerLengthBytesNeeded - bytesReceived;
            std::copy_n(bytes.begin(), bytesToParse,
                        std::back_inserter(headerBytes));
            std::memcpy(&headerSize, &headerBytes[headerLengthOffset],
                        sizeof(headerSize));
            parse_state = state::WAITING_FOR_HEADER;
            return bytesToParse;
        }
        std::copy(bytes.begin(), bytes.end(), std::back_inserter(headerBytes));
        return bytes.size();
    }
    if (parse_state == state::WAITING_FOR_HEADER)
    {
        // TODO: look into parsing in the header incrementally
        if (bytes.size() + bytesReceived >= headerSize)
        {
            size_t bytesToHeaderEnd = headerSize - bytesReceived;
            std::copy_n(bytes.begin(), bytesToHeaderEnd,
                        std::back_inserter(headerBytes));
            parse_state = state::PARSING_OUT_COMPONENTS;
            bool success = parseHeader();
            if (!success)
            {
                std::cerr << "failed to parse!\n";
                return std::nullopt;
            }
            return bytesToHeaderEnd;
        }
        std::copy(bytes.begin(), bytes.end(), std::back_inserter(headerBytes));
        return bytes.size();
    }

    if (parse_state == state::PARSING_OUT_COMPONENTS)
    {
        size_t bytesHandled = handoutFirmwareImage(bytes);
        if (bytesHandled + bytesReceived >= totalBytesToReceive)
        {
            parse_state = state::DONE;
        }
        return bytesHandled;
    }
    return std::nullopt;
}

bool PackageParser::processBytes(std::span<const uint8_t> bytes)
{
    // we keep calling updateStateMachine until we have no bytes left to process
    // updateStateMachine only parses the bytes that it can for each state
    // and returns the piece that was not handled. This allows us to handle
    // partial pieces of the header as they come in
    while (!bytes.empty() && parse_state != state::DONE)
    {
        std::optional<size_t> bytesRead = updateStateMachine(bytes);
        if (!bytesRead)
        {
            return false;
        }
        bytes = bytes.subspan(*bytesRead);
        bytesReceived += *bytesRead;
    }
    return true;
}
