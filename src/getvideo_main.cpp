#include <fcntl.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

//#define BUILD_CIMG
#ifdef BUILD_CIMG
#define cimg_display 0
#include <CImg.h>
#endif

#include <ast_jpeg_decoder.hpp>
#include <ast_video_puller.hpp>

int main()
{
    ast_video::RawVideoBuffer out;
    bool have_hardware = false;
    if (access("/dev/video", F_OK) != -1)
    {
        ast_video::SimpleVideoPuller p;
        p.initialize();
        out = p.readVideo();
    }
    else
    {
        FILE *fp = fopen("/home/ed/screendata.bin", "rb");
        if (fp != nullptr)
        {
            size_t newLen = fread(out.buffer.data(), sizeof(char),
                                  out.buffer.size() * sizeof(long), fp);
            if (ferror(fp) != 0)
            {
                fputs("Error reading file", stderr);
            }
            fclose(fp);
            out.buffer.resize(newLen);
            out.mode = ast_video::YuvMode::YUV444;
            out.width = 800;
            out.height = 600;
            out.ySelector = 0;
            out.uvSelector = 0;
        }
    }

    FILE *fp = fopen("/tmp/screendata.bin", "wb");
    fwrite(out.buffer.data(), sizeof(char), out.buffer.size(), fp);
    fclose(fp);

    ast_video::AstJpegDecoder d;
    d.decode(out.buffer, out.width, out.height, out.mode, out.ySelector,
             out.uvSelector);
#ifdef BUILD_CIMG
    cimg_library::CImg<unsigned char> image(out.width, out.height, 1,
                                            3 /*numchannels*/);
    for (int y = 0; y < out.height; y++)
    {
        for (int x = 0; x < out.width; x++)
        {
            auto pixel = d.outBuffer[x + (y * out.width)];
            image(x, y, 0) = pixel.r;
            image(x, y, 1) = pixel.g;
            image(x, y, 2) = pixel.b;
        }
    }
    image.save("/tmp/file2.bmp");
#endif

    std::cout << "Done!\n";

    return 1;
}
