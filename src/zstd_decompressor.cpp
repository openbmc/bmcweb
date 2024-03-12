#include "zstd_decompressor.hpp"

#include <zstd.h>

ZstdDecompressor::ZstdDecompressor() : dctx(ZSTD_createDCtx()) {}

std::optional<std::span<char>>
    ZstdDecompressor::decompress(const std::span<const char> buffIn)
{
    ZSTD_inBuffer input = {buffIn.data(), buffIn.size(), 0};

    ZSTD_outBuffer output = {compressionBuf.data(), compressionBuf.size(), 0};
    while (input.pos < input.size)
    {
        const size_t ret = ZSTD_decompressStream(dctx, &output, &input);
        if (ZSTD_isError(ret) != 0u)
        {
            return std::nullopt;
        }
    }
    return std::span<char>(static_cast<char*>(output.dst), output.pos);
}

ZstdDecompressor::~ZstdDecompressor()
{
    ZSTD_freeDCtx(dctx);
}
