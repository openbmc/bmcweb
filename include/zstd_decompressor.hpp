#include <zstd.h>

#include <array>
#include <optional>
#include <span>

class ZstdDecompressor
{
    std::array<char, ZSTD_BLOCKSIZE_MAX> compressionBuf{};

    ZSTD_DCtx* dctx;

  public:
    ZstdDecompressor();
    std::optional<std::span<char>> decompress(std::span<const char> buffIn);
    ~ZstdDecompressor();
};
