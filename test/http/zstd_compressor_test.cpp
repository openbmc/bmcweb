// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include <cstdint>
#include <vector>
#ifdef HAVE_ZSTD
#include "zstd_compressor.hpp"
#include "zstd_test_arrays.hpp"

#include <cstddef>
#include <optional>
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
    ZstdCompressor comp;
    ASSERT_TRUE(comp.init(1048576U));
    std::vector<uint8_t> out;
    std::vector<uint8_t> zeros(1024U, 0x00);
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

    EXPECT_THAT(out, ElementsAreArray(zstd::zeros));
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

} // namespace
} // namespace bmcweb
#endif
