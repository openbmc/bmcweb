#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <ast_jpeg_decoder.hpp>
#include <ast_video_puller.hpp>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(AstvideoPuller, BasicRead)
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
        FILE *fp =
            fopen("test_resources/ubuntu_444_800x600_0chrom_0lum.bin", "rb");
        if (fp)
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
}
