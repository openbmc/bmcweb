// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include <boost/asio/buffer.hpp>

#include <algorithm>
#include <climits>
#include <cstdint>
#include <functional>
#include <vector>
#ifdef HAVE_ZSTD
#include "zstd_compressor.hpp"
#include "zstd_decompressor.hpp"
#include "zstd_test_arrays.hpp"

#include <cstddef>
#include <optional>
#include <random>
#include <span>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::ElementsAreArray;

namespace bmcweb
{
namespace
{

TEST(ZstdCompressor, EmptyFile)
{
    ZstdCompressor comp;
    ASSERT_TRUE(comp.init(0U));
    std::vector<uint8_t> out;
    bool more = false;
    std::vector<uint8_t> empty;
    std::optional<std::span<const uint8_t>> segmentOut =
        comp.compress(empty, more);
    ASSERT_TRUE(segmentOut);
    if (!segmentOut)
    {
        return;
    }

    EXPECT_THAT(*segmentOut, ElementsAreArray(zstd::empty));
}

TEST(ZstdCompressor, AllZeros)
{
    for (size_t chunkSize : {1U, 2U, 4U, 8U, 1024U, 1048576U})
    {
        ZstdCompressor comp;
        constexpr size_t fileSize = 1048576U;
        ASSERT_TRUE(comp.init(fileSize));
        std::vector<uint8_t> out;

        std::vector<uint8_t> zeros(chunkSize, 0x00);
        for (size_t i = 0; i < fileSize; i += chunkSize)
        {
            bool more = i != fileSize - chunkSize;
            std::optional<std::span<const uint8_t>> segmentOut =
                comp.compress(zeros, more);
            ASSERT_TRUE(segmentOut);
            if (!segmentOut)
            {
                return;
            }
            out.insert(out.end(), segmentOut->begin(), segmentOut->end());
        }
        EXPECT_THAT(out, ElementsAreArray(zstd::zeros));
    }
}

TEST(ZstdCompressor, AllOnes)
{
    ZstdCompressor comp;
    ASSERT_TRUE(comp.init(1048576U));
    std::vector<uint8_t> out;
    std::vector<uint8_t> zeros(1024U, 0xFF);
    for (size_t i = 0; i < 1024U; i++)
    {
        bool more = i < 1023U;
        std::optional<std::span<const uint8_t>> segmentOut =
            comp.compress(zeros, more);
        ASSERT_TRUE(segmentOut);
        if (!segmentOut)
        {
            return;
        }
        out.insert(out.end(), segmentOut->begin(), segmentOut->end());
    }

    EXPECT_THAT(out, ElementsAreArray(zstd::ones));
}

TEST(Zstd, RoundTrip)
{
    using random_bytes_engine =
        std::independent_bits_engine<std::default_random_engine, CHAR_BIT,
                                     unsigned char>;

    // This is a unit test, we WANT reproducible tests
    // NOLINTNEXTLINE(cert-msc51-cpp, cert-msc32-c)
    random_bytes_engine rbe;
    std::vector<unsigned char> data(1048576U);
    std::ranges::generate(data, std::ref(rbe));

    for (size_t chunkSize : {1U, 2U, 4U, 8U, 1024U, 1048576U})
    {
        ZstdCompressor comp;
        std::vector<uint8_t> compressed;
        ASSERT_TRUE(comp.init(data.size()));
        for (size_t i = 0; i < data.size(); i += chunkSize)
        {
            bool more = i != data.size() - chunkSize;
            std::optional<std::span<const uint8_t>> segmentOut =
                comp.compress(std::span(data).subspan(i, chunkSize), more);
            ASSERT_TRUE(segmentOut);
            if (!segmentOut)
            {
                return;
            }
            compressed.insert(compressed.end(), segmentOut->begin(),
                              segmentOut->end());
        }
        ZstdDecompressor decomp;

        std::optional<boost::asio::const_buffer> segmentOut =
            decomp.decompress(boost::asio::buffer(compressed));
        ASSERT_TRUE(segmentOut);
        if (!segmentOut)
        {
            continue;
        }
        std::span<const uint8_t> decompressedSpan = std::span<const uint8_t>(
            static_cast<const uint8_t*>(segmentOut->data()),
            segmentOut->size());

        EXPECT_THAT(decompressedSpan, ElementsAreArray(data));
    }
}

} // namespace
} // namespace bmcweb
#endif
