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
               PrintToString(a) + " and " + PrintToString(b)) {
  return a <= arg && arg <= b;
};

TEST(AstJpegDecoder, AllBlue) {
  AstVideo::RawVideoBuffer out;

  // This binary blog was created on the aspeed hardware using a blue screen
  // consisting of the color 0x8EFFFA in a web browser window
  FILE *fp = fopen("test_resources/aspeedbluescreen.bin", "rb");
  EXPECT_NE(fp, nullptr);
  size_t bufferlen =
      fread(out.buffer.data(), sizeof(decltype(out.buffer)::value_type),
            out.buffer.size(), fp);
  fclose(fp);

  ASSERT_GT(bufferlen, 0);

  out.y_selector = 0;
  out.uv_selector = 0;
  out.mode = AstVideo::YuvMode::YUV444;
  out.width = 800;
  out.height = 600;

  AstVideo::AstJpegDecoder d;
  d.decode(out.buffer, out.width, out.height, out.mode, out.y_selector,
           out.uv_selector);

  int tolerance = 16;

  // All pixels should be blue (0x8EFFFA) to within a tolerance (due to jpeg
  // compression artifacts and quanitization)
  for (int i = 0; i < out.width * out.height; i++) {
    AstVideo::RGB &pixel = d.OutBuffer[i];
    EXPECT_GT(pixel.R, 0x8E - tolerance);
    EXPECT_LT(pixel.R, 0x8E + tolerance);
    EXPECT_GT(pixel.G, 0xFF - tolerance);
    EXPECT_LT(pixel.G, 0xFF + tolerance);
    EXPECT_GT(pixel.B, 0xF1 - tolerance);
    EXPECT_LT(pixel.B, 0xF1 + tolerance);
  }
}

TEST(AstJpegDecoder, AllBlack) {
  AstVideo::RawVideoBuffer out;

  // This binary blog was created on the aspeed hardware using a black screen
  FILE *fp = fopen("test_resources/aspeedblackscreen.bin", "rb");
  EXPECT_NE(fp, nullptr);
  size_t bufferlen = fread(out.buffer.data(), sizeof(char),
                           out.buffer.size() * sizeof(long), fp);
  fclose(fp);

  ASSERT_GT(bufferlen, 0);

  out.y_selector = 0;
  out.uv_selector = 0;
  out.mode = AstVideo::YuvMode::YUV444;
  out.width = 800;
  out.height = 600;

  AstVideo::AstJpegDecoder d;
  d.decode(out.buffer, out.width, out.height, out.mode, out.y_selector,
           out.uv_selector);

  // All pixels should be blue (0x8EFFFA) to within a tolerance (due to jpeg
  // compression artifacts and quanitization)
  for (int x = 0; x < out.width; x++) {
    for (int y = 0; y < out.height; y++) {
      AstVideo::RGB pixel = d.OutBuffer[x + (y * out.width)];
      ASSERT_EQ(pixel.R, 0x00) << "X:" << x << " Y: " << y;
      ASSERT_EQ(pixel.G, 0x00) << "X:" << x << " Y: " << y;
      ASSERT_EQ(pixel.B, 0x00) << "X:" << x << " Y: " << y;
    }
  }
}

TEST(AstJpegDecoder, TestColors) {
  AstVideo::RawVideoBuffer out;

  // This binary blog was created on the aspeed hardware using a blue screen
  // consisting of the color 0x8EFFFA in a web browser window
  FILE *fp = fopen("test_resources/ubuntu_444_800x600_0chrom_0lum.bin", "rb");
  EXPECT_NE(fp, nullptr);
  size_t bufferlen = fread(out.buffer.data(), sizeof(char),
                           out.buffer.size() * sizeof(long), fp);
  fclose(fp);

  ASSERT_GT(bufferlen, 0);

  out.y_selector = 0;
  out.uv_selector = 0;
  out.mode = AstVideo::YuvMode::YUV444;
  out.width = 800;
  out.height = 600;

  AstVideo::AstJpegDecoder d;
  d.decode(out.buffer, out.width, out.height, out.mode, out.y_selector,
           out.uv_selector);

  int tolerance = 16;
  /*
  for (int i = 0; i < out.width * out.height; i++) {
    AstVideo::RGB &pixel = d.OutBuffer[i];
    EXPECT_GT(pixel.R, 0x8E - tolerance);
    EXPECT_LT(pixel.R, 0x8E + tolerance);
    EXPECT_GT(pixel.G, 0xFF - tolerance);
    EXPECT_LT(pixel.G, 0xFF + tolerance);
    EXPECT_GT(pixel.B, 0xF1 - tolerance);
    EXPECT_LT(pixel.B, 0xF1 + tolerance);
  }
  */
}

// Tests the buffers around the screen aren't written to
TEST(AstJpegDecoder, BufferLimits) {
  AstVideo::RawVideoBuffer out;

  // This binary blog was created on the aspeed hardware using a black screen
  FILE *fp = fopen("test_resources/aspeedblackscreen.bin", "rb");
  EXPECT_NE(fp, nullptr);
  size_t bufferlen = fread(out.buffer.data(), sizeof(char),
                           out.buffer.size() * sizeof(long), fp);
  fclose(fp);

  ASSERT_GT(bufferlen, 0);

  out.y_selector = 0;
  out.uv_selector = 0;
  out.mode = AstVideo::YuvMode::YUV444;
  out.width = 800;
  out.height = 600;

  AstVideo::AstJpegDecoder d;
  d.decode(out.buffer, out.width, out.height, out.mode, out.y_selector,
           out.uv_selector);
  // Reserved pixel should be default value
  for (auto &pixel : d.OutBuffer) {
    EXPECT_EQ(pixel.Reserved, 0xAA);
  }
  // All pixels beyond the buffer should be zero
  for (int i = out.width * out.height; i < d.OutBuffer.size(); i++) {
    EXPECT_EQ(d.OutBuffer[i].R, 0x00) << "index:" << i;
    EXPECT_EQ(d.OutBuffer[i].B, 0x00) << "index:" << i;
    EXPECT_EQ(d.OutBuffer[i].G, 0x00) << "index:" << i;
  }
}