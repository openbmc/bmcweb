// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include <cstdint>
#ifdef HAVE_ZSTD
#include "zstd_decompressor.hpp"
#include "zstd_test_arrays.hpp"

#include <boost/asio/buffer.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <span>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::Each;
using ::testing::Eq;

namespace bmcweb
{
namespace
{

TEST(Zstd, EmptyFile)
{
    ZstdDecompressor comp;
    std::optional<boost::asio::const_buffer> out =
        comp.decompress(boost::asio::buffer(zstd::empty));
    ASSERT_TRUE(out);
    if (!out)
    {
        return;
    }
    EXPECT_TRUE(out->size() == 0);
}

TEST(Zstd, ZerosFile)
{
    for (size_t chunkSize :
         std::to_array<size_t>({1U, 2U, 4U, 8U, 16U, zstd::zeros.size()}))
    {
        ZstdDecompressor comp;
        std::span<const uint8_t> data(zstd::zeros);
        size_t read = 0;
        while (!data.empty())
        {
            std::span<const uint8_t> chunk =
                data.subspan(0, std::min(chunkSize, data.size()));
            std::optional<boost::asio::const_buffer> out = comp.decompress(
                boost::asio::buffer(chunk.data(), chunk.size()));
            ASSERT_TRUE(out);
            if (out)
            {
                EXPECT_THAT(std::span(static_cast<const uint8_t*>(out->data()),
                                      out->size()),
                            Each(Eq(0)));
                read += out->size();
            }
            data = data.subspan(chunk.size());
        }

        EXPECT_EQ(read, 1024 * 1024);
    }
}

TEST(Zstd, OnesFile)
{
    for (size_t chunkSize :
         std::to_array<size_t>({1U, 2U, 4U, 8U, 16U, zstd::ones.size()}))
    {
        ZstdDecompressor comp;
        std::span<const uint8_t> data = std::span(zstd::ones);
        size_t read = 0;
        while (!data.empty())
        {
            std::span<const uint8_t> chunk =
                data.subspan(0, std::min(chunkSize, data.size()));
            std::optional<boost::asio::const_buffer> out = comp.decompress(
                boost::asio::buffer(chunk.data(), chunk.size()));
            ASSERT_TRUE(out);
            if (out)
            {
                EXPECT_THAT(std::span(static_cast<const uint8_t*>(out->data()),
                                      out->size()),
                            Each(Eq(0xFF)));
                read += out->size();
            }
            data = data.subspan(chunk.size());
        }

        EXPECT_EQ(read, 1024 * 1024);
    }
}

} // namespace
} // namespace bmcweb
#endif
