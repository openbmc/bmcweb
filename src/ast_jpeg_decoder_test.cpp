#include "ast_jpeg_decoder.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#ifdef BUILD_CIMG
#define cimg_display 0
#include <CImg.h>
#endif

using namespace testing;
MATCHER_P2(IsBetween, a, b,
           std::string(negation ? "isn't" : "is") + " between " +
               PrintToString(a) + " and " + PrintToString(b))
{
    return a <= arg && arg <= b;
};

TEST(AstJpegDecoder, AllBlue)
{
    ast_video::RawVideoBuffer out;

    // This binary blog was created on the aspeed hardware using a blue screen
    // consisting of the color 0x8EFFFA in a web browser window
    FILE *fp = fopen("test_resources/aspeedbluescreen.bin", "rb");
    EXPECT_NE(fp, nullptr);
    size_t bufferlen =
        fread(out.buffer.data(), sizeof(decltype(out.buffer)::value_type),
              out.buffer.size(), fp);
    fclose(fp);

    ASSERT_GT(bufferlen, 0);

    out.ySelector = 0;
    out.uvSelector = 0;
    out.mode = ast_video::YuvMode::YUV444;
    out.width = 800;
    out.height = 600;

    ast_video::AstJpegDecoder d;
    d.decode(out.buffer, out.width, out.height, out.mode, out.ySelector,
             out.uvSelector);

    int tolerance = 16;

    // All pixels should be blue (0x8EFFFA) to within a tolerance (due to jpeg
    // compression artifacts and quanitization)
    for (int i = 0; i < out.width * out.height; i++)
    {
        ast_video::RGB &pixel = d.outBuffer[i];
        EXPECT_GT(pixel.r, 0x8E - tolerance);
        EXPECT_LT(pixel.r, 0x8E + tolerance);
        EXPECT_GT(pixel.g, 0xFF - tolerance);
        EXPECT_LT(pixel.g, 0xFF + tolerance);
        EXPECT_GT(pixel.b, 0xF1 - tolerance);
        EXPECT_LT(pixel.b, 0xF1 + tolerance);
    }
}

TEST(AstJpegDecoder, AllBlack)
{
    ast_video::RawVideoBuffer out;

    // This binary blog was created on the aspeed hardware using a black screen
    FILE *fp = fopen("test_resources/aspeedblackscreen.bin", "rb");
    EXPECT_NE(fp, nullptr);
    size_t bufferlen = fread(out.buffer.data(), sizeof(char),
                             out.buffer.size() * sizeof(long), fp);
    fclose(fp);

    ASSERT_GT(bufferlen, 0);

    out.ySelector = 0;
    out.uvSelector = 0;
    out.mode = ast_video::YuvMode::YUV444;
    out.width = 800;
    out.height = 600;

    ast_video::AstJpegDecoder d;
    d.decode(out.buffer, out.width, out.height, out.mode, out.ySelector,
             out.uvSelector);

    // All pixels should be blue (0x8EFFFA) to within a tolerance (due to jpeg
    // compression artifacts and quanitization)
    for (int x = 0; x < out.width; x++)
    {
        for (int y = 0; y < out.height; y++)
        {
            ast_video::RGB pixel = d.outBuffer[x + (y * out.width)];
            ASSERT_EQ(pixel.r, 0x00) << "X:" << x << " Y: " << y;
            ASSERT_EQ(pixel.g, 0x00) << "X:" << x << " Y: " << y;
            ASSERT_EQ(pixel.b, 0x00) << "X:" << x << " Y: " << y;
        }
    }
}

TEST(AstJpegDecoder, TestColors)
{
    ast_video::RawVideoBuffer out;

    // This binary blog was created on the aspeed hardware using a blue screen
    // consisting of the color 0x8EFFFA in a web browser window
    FILE *fp = fopen("test_resources/ubuntu_444_800x600_0chrom_0lum.bin", "rb");
    EXPECT_NE(fp, nullptr);
    size_t bufferlen = fread(out.buffer.data(), sizeof(char),
                             out.buffer.size() * sizeof(long), fp);
    fclose(fp);

    ASSERT_GT(bufferlen, 0);

    out.ySelector = 0;
    out.uvSelector = 0;
    out.mode = ast_video::YuvMode::YUV444;
    out.width = 800;
    out.height = 600;

    ast_video::AstJpegDecoder d;
    d.decode(out.buffer, out.width, out.height, out.mode, out.ySelector,
             out.uvSelector);

    int tolerance = 16;
    /*
    for (int i = 0; i < out.width * out.height; i++) {
      ast_video::RGB &pixel = d.outBuffer[i];
      EXPECT_GT(pixel.r, 0x8E - tolerance);
      EXPECT_LT(pixel.r, 0x8E + tolerance);
      EXPECT_GT(pixel.g, 0xFF - tolerance);
      EXPECT_LT(pixel.g, 0xFF + tolerance);
      EXPECT_GT(pixel.b, 0xF1 - tolerance);
      EXPECT_LT(pixel.b, 0xF1 + tolerance);
    }
    */
}

// Tests the buffers around the screen aren't written to
TEST(AstJpegDecoder, BufferLimits)
{
    ast_video::RawVideoBuffer out;

    // This binary blog was created on the aspeed hardware using a black screen
    FILE *fp = fopen("test_resources/aspeedblackscreen.bin", "rb");
    EXPECT_NE(fp, nullptr);
    size_t bufferlen = fread(out.buffer.data(), sizeof(char),
                             out.buffer.size() * sizeof(long), fp);
    fclose(fp);

    ASSERT_GT(bufferlen, 0);

    out.ySelector = 0;
    out.uvSelector = 0;
    out.mode = ast_video::YuvMode::YUV444;
    out.width = 800;
    out.height = 600;

    ast_video::AstJpegDecoder d;
    d.decode(out.buffer, out.width, out.height, out.mode, out.ySelector,
             out.uvSelector);
    // reserved pixel should be default value
    for (auto &pixel : d.outBuffer)
    {
        EXPECT_EQ(pixel.reserved, 0xAA);
    }
    // All pixels beyond the buffer should be zero
    for (int i = out.width * out.height; i < d.outBuffer.size(); i++)
    {
        EXPECT_EQ(d.outBuffer[i].r, 0x00) << "index:" << i;
        EXPECT_EQ(d.outBuffer[i].b, 0x00) << "index:" << i;
        EXPECT_EQ(d.outBuffer[i].g, 0x00) << "index:" << i;
    }
}