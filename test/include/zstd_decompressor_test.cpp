#include "logging.hpp"
#include "zstd_decompressor.hpp"

#include <boost/asio/buffer.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::Each;
using ::testing::Eq;

namespace zstd
{
namespace
{

TEST(Zstd, EmptyFile)
{
    std::array<unsigned char, 13> empty{0x28, 0xb5, 0x2f, 0xfd, 0x24,
                                        0x00, 0x01, 0x00, 0x00, 0x99,
                                        0xe9, 0xd8, 0x51};

    ZstdDecompressor comp;
    std::optional<boost::asio::const_buffer> out =
        comp.decompress(boost::asio::buffer(empty));
    ASSERT_TRUE(out);
    if (!out)
    {
        return;
    }
    EXPECT_TRUE(out->size() == 0);
}

TEST(Zstd, ZerosFile)
{
    // A 1MB file of all zeros created using
    // dd if=/dev/zero of=zeros-file bs=1024 count=1024
    // zstd -c zeros-file | xxd -i
    std::array<unsigned char, 54> zeros = {
        0x28, 0xb5, 0x2f, 0xfd, 0xa4, 0x00, 0x00, 0x10, 0x00, 0x54, 0x00,
        0x00, 0x10, 0x00, 0x00, 0x01, 0x00, 0xfb, 0xff, 0x39, 0xc0, 0x02,
        0x02, 0x00, 0x10, 0x00, 0x02, 0x00, 0x10, 0x00, 0x02, 0x00, 0x10,
        0x00, 0x02, 0x00, 0x10, 0x00, 0x02, 0x00, 0x10, 0x00, 0x02, 0x00,
        0x10, 0x00, 0x03, 0x00, 0x10, 0x00, 0xf1, 0x3e, 0x16, 0xe1};

    for (size_t chunkSize :
         std::to_array<size_t>({1U, 2U, 4U, 8U, 16U, zeros.size()}))
    {
        ZstdDecompressor comp;
        std::span<unsigned char> data = std::span(zeros);
        size_t read = 0;
        while (!data.empty())
        {
            std::span<unsigned char> chunk =
                data.subspan(0, std::min(chunkSize, data.size()));
            std::optional<boost::asio::const_buffer> out = comp.decompress(
                boost::asio::buffer(chunk.data(), chunk.size()));
            ASSERT_TRUE(out);
            if (out)
            {
                EXPECT_THAT(
                    std::span(static_cast<const unsigned char*>(out->data()),
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
    // A 1MB file of all zeros created using
    // dd if=/dev/zero bs=1024 count=1024 | tr "\000" "\377" > ones.txt
    // zstd -c ones-file | xxd -i
    std::array<unsigned char, 54> ones = {
        0x28, 0xb5, 0x2f, 0xfd, 0xa4, 0x00, 0x00, 0x10, 0x00, 0x54, 0x00,
        0x00, 0x10, 0xff, 0xff, 0x01, 0x00, 0xfb, 0xff, 0x39, 0xc0, 0x02,
        0x02, 0x00, 0x10, 0xff, 0x02, 0x00, 0x10, 0xff, 0x02, 0x00, 0x10,
        0xff, 0x02, 0x00, 0x10, 0xff, 0x02, 0x00, 0x10, 0xff, 0x02, 0x00,
        0x10, 0xff, 0x03, 0x00, 0x10, 0xff, 0xb4, 0xc8, 0xba, 0x13};

    for (size_t chunkSize :
         std::to_array<size_t>({1U, 2U, 4U, 8U, 16U, ones.size()}))
    {
        ZstdDecompressor comp;
        std::span<unsigned char> data = std::span(ones);
        size_t read = 0;
        while (!data.empty())
        {
            std::span<unsigned char> chunk =
                data.subspan(0, std::min(chunkSize, data.size()));
            std::optional<boost::asio::const_buffer> out = comp.decompress(
                boost::asio::buffer(chunk.data(), chunk.size()));
            ASSERT_TRUE(out);
            if (out)
            {
                EXPECT_THAT(
                    std::span(static_cast<const unsigned char*>(out->data()),
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
} // namespace zstd
