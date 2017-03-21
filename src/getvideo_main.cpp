#include <video.h>
#include <fstream>
#include <iostream>
#include <iomanip>

namespace AstVideo {
class VideoPuller {
 public:
  VideoPuller() {}

  void initialize() {
    std::cout << "Opening /dev/video\n";
    file.open("/dev/video", std::ios::out | std::ios::in | std::ios::binary);
    if (!file.is_open()) {
      std::cout << "Failed to open /dev/video\n";
    }
    IMAGE_INFO image_info{};

    file.write(reinterpret_cast<char*>(&image_info), sizeof(image_info));

    file.read(reinterpret_cast<char*>(&image_info), sizeof(image_info));
    
    if (file){
        std::cout << "Read succeeded\n";
    }
    
    auto pt = reinterpret_cast<char*>(&image_info);

    for(int i=0; i<sizeof(image_info); i++){
        std::cout << std::hex << std::setfill('0') << std::setw(2) << int(*(pt + i)) << " ";
    }
    
    std::cout << "\n";

    std::cout << "typedef struct _video_features {\n";
    std::cout << "short jpg_fmt: " << image_info.parameter.features.jpg_fmt << "\n";
    std::cout << "short lumin_tbl;" << image_info.parameter.features.lumin_tbl << "\n";
    std::cout << "short chrom_tbl;" << image_info.parameter.features.chrom_tbl << "\n";
    std::cout << "short tolerance_noise;" << image_info.parameter.features.tolerance_noise << "\n";
    std::cout << "int w;" << image_info.parameter.features.w << "\n";
    std::cout << "int h;" << image_info.parameter.features.h << "\n";
    //std::cout << "unsigned char *buf;" << image_info.parameter.features.buf << "\n";
    std::cout << "} FEATURES_TAG;\n";

    std::cout << "typedef struct _image_info {";
    std::cout << "short do_image_refresh;" << image_info.do_image_refresh << "\n";
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
  }
  std::fstream file;
};
}

int main() {
  AstVideo::VideoPuller p;
  p.initialize();

  return 1;
}