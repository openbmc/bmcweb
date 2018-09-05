#pragma once

#include <cstdint>
#include <vector>
namespace ast_video
{
enum class YuvMode
{
    YUV444 = 0,
    YUV420 = 1
};

class RawVideoBuffer
{
  public:
    RawVideoBuffer() : buffer(1024 * 1024 * 10, 0){};
    unsigned long height{};
    unsigned long width{};
    int ySelector{};
    int uvSelector{};
    YuvMode mode;
    // TODO(ed) determine a more appropriate buffer size
    std::vector<uint32_t> buffer;
};
} // namespace ast_video