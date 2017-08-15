#include <fcntl.h>
#include <unistd.h>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#include <cstdio>
#include <cstdlib>

//#define BUILD_CIMG
#ifdef BUILD_CIMG
#define cimg_display 0
#include <CImg.h>
#endif

#include <ast_jpeg_decoder.hpp>
#include <ast_video_puller.hpp>

int main() {
  AstVideo::RawVideoBuffer out;
  bool have_hardware = false;
  if (access("/dev/video", F_OK) != -1) {
    AstVideo::SimpleVideoPuller p;
    p.initialize();
    out = p.read_video();
  } else {
    FILE *fp = fopen("/home/ed/screendata.bin", "rb");
    if (fp != nullptr) {
      size_t newLen = fread(out.buffer.data(), sizeof(char),
                            out.buffer.size() * sizeof(long), fp);
      if (ferror(fp) != 0) {
        fputs("Error reading file", stderr);
      }
      fclose(fp);
      out.buffer.resize(newLen);
      out.mode = AstVideo::YuvMode::YUV444;
      out.width = 800;
      out.height = 600;
      out.y_selector = 0;
      out.uv_selector = 0;
    }
  }

  FILE *fp = fopen("/tmp/screendata.bin", "wb");
  fwrite(out.buffer.data(), sizeof(char), out.buffer.size(), fp);

  AstVideo::AstJpegDecoder d;
  d.decode(out.buffer, out.width, out.height, out.mode, out.y_selector,
           out.uv_selector);
#ifdef BUILD_CIMG
  cimg_library::CImg<unsigned char> image(out.width, out.height, 1,
                                          3 /*numchannels*/);
  for (int y = 0; y < out.height; y++) {
    for (int x = 0; x < out.width; x++) {
      auto pixel = d.OutBuffer[x + (y * out.width)];
      image(x, y, 0) = pixel.R;
      image(x, y, 1) = pixel.G;
      image(x, y, 2) = pixel.B;
    }
  }
  image.save("/tmp/file2.bmp");
#endif

  std::cout << "Done!\n";

  return 1;
}
