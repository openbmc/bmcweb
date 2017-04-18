#pragma once

#include <assert.h>
//#include <video.h>
#include <ast_video_types.hpp>
#include <iostream>
#include <vector>

namespace AstVideo {
class VideoPuller {
  //
  // Cursor struct is used in User Mode
  //
  typedef struct _cursor_attribution_tag {
    unsigned int posX;
    unsigned int posY;
    unsigned int cur_width;
    unsigned int cur_height;
    unsigned int cur_type;  // 0:mono 1:color 2:disappear cursor
    unsigned int cur_change_flag;
  } AST_CUR_ATTRIBUTION_TAG;

  //
  // For storing Cursor Information
  //
  typedef struct _cursor_tag {
    AST_CUR_ATTRIBUTION_TAG attr;
    // unsigned char     icon[MAX_CUR_OFFSETX*MAX_CUR_OFFSETY*2];
    unsigned char *icon;  //[64*64*2];
  } AST_CURSOR_TAG;

  //
  // For select image format, i.e. 422 JPG420, 444 JPG444, lumin/chrom table, 0
  // ~ 11, low to high
  //
  typedef struct _video_features {
    short jpg_fmt;  // 422:JPG420, 444:JPG444
    short lumin_tbl;
    short chrom_tbl;
    short tolerance_noise;
    int w;
    int h;
    unsigned char *buf;
  } FEATURES_TAG;

  //
  // For configure video engine control registers
  //
  typedef struct _image_info {
    short do_image_refresh;  // Action 0:motion 1:fullframe 2:quick cursor
    char qc_valid;           // quick cursor enable/disable
    unsigned int len;
    int crypttype;
    char cryptkey[16];
    union {
      FEATURES_TAG features;
      AST_CURSOR_TAG cursor_info;
    } parameter;
  } IMAGE_INFO;

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
