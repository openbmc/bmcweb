#pragma once

#include <cassert>
#include <ast_video_types.hpp>
#include <iostream>
#include <mutex>
#include <vector>
#include <boost/asio.hpp>

namespace AstVideo {

//
// Cursor struct is used in User Mode
//
struct AST_CUR_ATTRIBUTION_TAG {
  unsigned int posX;
  unsigned int posY;
  unsigned int cur_width;
  unsigned int cur_height;
  unsigned int cur_type;  // 0:mono 1:color 2:disappear cursor
  unsigned int cur_change_flag;
};

//
// For storing Cursor Information
//
struct AST_CURSOR_TAG {
  AST_CUR_ATTRIBUTION_TAG attr;
  // unsigned char     icon[MAX_CUR_OFFSETX*MAX_CUR_OFFSETY*2];
  unsigned char *icon;  //[64*64*2];
};

//
// For select image format, i.e. 422 JPG420, 444 JPG444, lumin/chrom table, 0
// ~ 11, low to high
//
struct FEATURES_TAG {
  short jpg_fmt;  // 422:JPG420, 444:JPG444
  short lumin_tbl;
  short chrom_tbl;
  short tolerance_noise;
  int w;
  int h;
  unsigned char *buf;
};

//
// For configure video engine control registers
//
struct IMAGE_INFO {
  short do_image_refresh;  // Action 0:motion 1:fullframe 2:quick cursor
  char qc_valid;           // quick cursor enable/disable
  unsigned int len;
  int crypttype;
  char cryptkey[16];
  union {
    FEATURES_TAG features;
    AST_CURSOR_TAG cursor_info;
  } parameter;
};

class SimpleVideoPuller {
 public:
  SimpleVideoPuller() : image_info(){};

  void initialize() {
    std::cout << "Opening /dev/video\n";
    video_fd = open("/dev/video", O_RDWR);
    if (video_fd == 0) {
      std::cout << "Failed to open /dev/video\n";
      throw std::runtime_error("Failed to open /dev/video");
    }
    std::cout << "Opened successfully\n";
  }

  RawVideoBuffer read_video() {
    assert(video_fd != 0);
    RawVideoBuffer raw;

    image_info.do_image_refresh = 1;  // full frame refresh
    image_info.qc_valid = 0;          // quick cursor disabled
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
    std::cout << "Done reading\n";

    if (status != 0) {
      std::cerr << "Read failed with status " << status << "\n";
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
  int video_fd{};
  IMAGE_INFO image_info;
};

#if defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
class AsyncVideoPuller {
 public:
  using video_callback = std::function<void (RawVideoBuffer &)>;

  explicit AsyncVideoPuller(boost::asio::io_service &io_service)
      : image_info(), dev_video(io_service, open("/dev/video", O_RDWR)) {
    videobuf = std::make_shared<RawVideoBuffer>();

    image_info.do_image_refresh = 1;  // full frame refresh
    image_info.qc_valid = 0;          // quick cursor disabled
    image_info.parameter.features.buf =
        reinterpret_cast<unsigned char *>(videobuf->buffer.data());
    image_info.crypttype = -1;
  };

  void register_callback(video_callback &callback) {
    std::lock_guard<std::mutex> lock(callback_mutex);
    callbacks.push_back(callback);
    start_read();
  }

  void start_read() {
    auto mutable_buffer = boost::asio::buffer(&image_info, sizeof(image_info));
    boost::asio::async_read(
        dev_video, mutable_buffer, [this](const boost::system::error_code &ec,
                                          std::size_t bytes_transferred) {
          if (ec != nullptr) {
            std::cerr << "Read failed with status " << ec << "\n";
          } else {
            this->read_done();
          }
        });
  }

  void read_done() {
    std::cout << "Done reading\n";
    videobuf->buffer.resize(image_info.len);

    videobuf->height = image_info.parameter.features.h;
    videobuf->width = image_info.parameter.features.w;
    if (image_info.parameter.features.jpg_fmt == 422) {
      videobuf->mode = YuvMode::YUV420;
    } else {
      videobuf->mode = YuvMode::YUV444;
    }
    std::lock_guard<std::mutex> lock(callback_mutex);
    for (auto &callback : callbacks) {
      // TODO(ed) call callbacks async and double buffer frames
      callback(*videobuf);
    }
  }

 private:
  std::shared_ptr<RawVideoBuffer> videobuf;
  boost::asio::posix::stream_descriptor dev_video;
  IMAGE_INFO image_info;
  std::mutex callback_mutex;
  std::vector<video_callback> callbacks;
};
#endif  // defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
} // namespace AstVideo
