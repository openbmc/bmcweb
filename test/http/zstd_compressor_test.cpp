// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#ifdef HAVE_ZSTD
#include "utils/hex_utils.hpp"
#include "zstd_compressor.hpp"
#include "zstd_test_arrays.hpp"

#include <boost/asio/buffer.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <span>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::ElementsAreArray;
using ::testing::IsEmpty;

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
    std::optional<std::span<const uint8_t>> segment_out =
        comp.compress(empty, more);
    ASSERT_TRUE(segment_out);
    if (!segment_out)
    {
        return;
    }

    EXPECT_THAT(*segment_out, ElementsAreArray(zstd::empty));
}

TEST(ZstdCompressor, AllZeros)
{
    ZstdCompressor comp;
    ASSERT_TRUE(comp.init(1024U * 1024U));
    std::vector<uint8_t> out;
    std::vector<uint8_t> zeros(1024U, 0x00);
    for (size_t i = 0; i < 1024U; i++)
    {
        bool more = i < 1023U;
        std::optional<std::span<const uint8_t>> segment_out =
            comp.compress(zeros, more);
        ASSERT_TRUE(segment_out);
        if (!segment_out)
        {
            return;
        }
        out.insert(out.end(), segment_out->begin(), segment_out->end());
    }

    EXPECT_THAT(out, ElementsAreArray(zstd::zeros));
}

TEST(ZstdCompressor, AllOnes)
{
    ZstdCompressor comp;
    ASSERT_TRUE(comp.init(1024U * 1024U));
    std::vector<uint8_t> out;
    std::vector<uint8_t> zeros(1024U, 0xFF);
    for (size_t i = 0; i < 1024U; i++)
    {
        bool more = i < 1023U;
        std::optional<std::span<const uint8_t>> segment_out =
            comp.compress(zeros, more);
        ASSERT_TRUE(segment_out);
        if (!segment_out)
        {
            return;
        }
        out.insert(out.end(), segment_out->begin(), segment_out->end());
    }

    EXPECT_THAT(out, ElementsAreArray(zstd::ones));
}

} // namespace
} // namespace bmcweb
#endif
