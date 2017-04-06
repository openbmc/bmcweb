#include <video.h>

#include <iomanip>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>


namespace AstVideo {
class VideoPuller {
 public:
  VideoPuller() {}

  void initialize() {
    std::cout << "Opening /dev/video\n";
    video_fd = open("/dev/video", O_RDWR);
    if (!video_fd) {
      std::cout << "Failed to open /dev/video\n";
    } else {
      std::cout << "Opened successfully\n";
    }

    std::vector<unsigned char> buffer(1024 * 1024, 0);

    IMAGE_INFO image_info{};
    image_info.do_image_refresh = 1;  // full frame refresh
    image_info.qc_valid = 0;          // quick cursor disabled
    image_info.parameter.features.w = 800;
    image_info.parameter.features.h = 600;
    image_info.parameter.features.chrom_tbl = 0;  // level
    image_info.parameter.features.lumin_tbl = 0;
    image_info.parameter.features.jpg_fmt = 1;
    image_info.parameter.features.buf = buffer.data();
    image_info.crypttype = -1;
    std::cout << "Writing\n";
    
    int status;
    
    status = write(video_fd, reinterpret_cast<char*>(&image_info),
                        sizeof(image_info));
    if (status != 0) {
      std::cout << "Write failed.  Return: " << status <<"\n";
      perror("perror output:");
    }
    
    std::cout << "Write done\n";
    //std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    
    std::cout << "Reading\n";
    status = read(video_fd, reinterpret_cast<char*>(&image_info), sizeof(image_info));
    std::cout << "Reading\n";

    if (status != 0) {
      std::cout << "Read failed with status " << status << "\n";
    }

    auto pt = reinterpret_cast<char*>(&image_info);

    for (int i = 0; i < sizeof(image_info); i++) {
      std::cout << std::hex << std::setfill('0') << std::setw(2)
                << int(*(pt + i)) << " ";
    }

    std::cout << "\nprinting buffer\n";
    
    for(int i = 0; i < 512; i++){
        if (i % 16 == 0){
          std::cout << "\n";
        }
        std::cout << std::hex << std::setfill('0') << std::setw(2)
            << int(buffer[i]) << " ";
    }
    
    buffer.resize(image_info.len);
    
    std::ofstream f("/tmp/screen.jpg",std::ios::out | std::ios::binary); 

    f.write(reinterpret_cast<char*>(buffer.data()), buffer.size());

    std::cout << "\n";

    std::cout << "typedef struct _video_features {\n";
    std::cout << "short jpg_fmt: " << image_info.parameter.features.jpg_fmt
              << "\n";
    std::cout << "short lumin_tbl;" << image_info.parameter.features.lumin_tbl
              << "\n";
    std::cout << "short chrom_tbl;" << image_info.parameter.features.chrom_tbl
              << "\n";
    std::cout << "short tolerance_noise;"
              << image_info.parameter.features.tolerance_noise << "\n";
    std::cout << "int w; 0X" << image_info.parameter.features.w << "\n";
    std::cout << "int h; 0X" << image_info.parameter.features.h << "\n";

    std::cout << "void* buf; 0X" << static_cast<void*>(image_info.parameter.features.buf) << "\n";
    // std::cout << "unsigned char *buf;" << image_info.parameter.features.buf
    // << "\n";
    std::cout << "} FEATURES_TAG;\n";

    std::cout << "typedef struct _image_info {";
    std::cout << "short do_image_refresh;" << image_info.do_image_refresh
              << "\n";
    std::cout << "char qc_valid;" << image_info.qc_valid << "\n";
    std::cout << "unsigned int len;" << image_info.len << "\n";
    std::cout << "int crypttype;" << image_info.crypttype << "\n";
    std::cout << "char cryptkey[16];" << image_info.cryptkey << "\n";
    std::cout << "union {\n";
    std::cout << "    FEATURES_TAG features;\n";
    std::cout << "    AST_CURSOR_TAG cursor_info;\n";
    std::cout << "} parameter;\n";
    std::cout << "} IMAGE_INFO;\n";
    std::cout << std::endl;

    close(video_fd);
  }
  int video_fd;
};
}

int main() {
  AstVideo::VideoPuller p;
  p.initialize();

  return 1;
}