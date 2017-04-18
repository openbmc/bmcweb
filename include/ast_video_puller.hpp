#pragma once

#include <vector>
#include <assert.h>

#include <ast_video_types.hpp>

#include <video.h>
#include <iostream>

namespace AstVideo {
class VideoPuller {
 public:
  VideoPuller() : image_info(){};

  void initialize() {
    std::cout << "Opening /dev/video\n";
    video_fd = open("/dev/video", O_RDWR);
    if (!video_fd) {
      std::cout << "Failed to open /dev/video\n";
      // TODO(Ed) throw exception?
    } else {
      std::cout << "Opened successfully\n";
    }
  }

  RawVideoBuffer read_video() {
    assert(video_fd != 0);
    RawVideoBuffer raw;

    IMAGE_INFO image_info{};
    image_info.do_image_refresh = 1;  // full frame refresh
    image_info.qc_valid = 0;          // quick cursor disabled
    image_info.parameter.features.w = 0;
    image_info.parameter.features.h = 0;
    image_info.parameter.features.chrom_tbl = 0;  // level
    image_info.parameter.features.lumin_tbl = 0;
    image_info.parameter.features.jpg_fmt = 1;
    image_info.parameter.features.buf =
        reinterpret_cast<unsigned char *>(raw.buffer.data());
    image_info.crypttype = -1;
    std::cout << "Writing\n";

    int status;
    /*
    status = write(video_fd, reinterpret_cast<char*>(&image_info),
                        sizeof(image_info));
    if (status != sizeof(image_info)) {
      std::cout << "Write failed.  Return: " << status << "\n";
      perror("perror output:");
    }
    
    std::cout << "Write done\n";
    */
    std::cout << "Reading\n";
    status = read(video_fd, reinterpret_cast<char *>(&image_info),
                  sizeof(image_info));
    std::cout << "Reading\n";

    if (status != 0) {
      std::cout << "Read failed with status " << status << "\n";
    }

    raw.buffer.resize(image_info.len);

    raw.height = image_info.parameter.features.h;
    raw.width = image_info.parameter.features.w;
    if (image_info.parameter.features.jpg_fmt == 422) {
      raw.mode = YuvMode::YUV420;
    } else {
      raw.mode = YuvMode::YUV444;
    }
    return raw;
  }

 private:
  int video_fd;
  IMAGE_INFO image_info;
};
}
