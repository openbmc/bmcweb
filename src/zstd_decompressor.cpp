#include "zstd_decompressor.hpp"

#include "logging.hpp"

#include <zstd.h>

ZstdDecompressor::ZstdDecompressor() : dctx(ZSTD_createDStream())
{
    ZSTD_initDStream(dctx);
}

std::optional<boost::asio::const_buffer>
    ZstdDecompressor::decompress(boost::asio::const_buffer buffIn)
{
    compressionBuf.clear();
    ZSTD_inBuffer input = {buffIn.data(), buffIn.size(), 0};

    // Note, this loop is prone to compression bombs, decompressing chunks that
    // appear very small, but decompress to be very large, given that they're
    // highly decompressible. This algorithm assumes that at this time, the
    // whole file will fit in ram.
    while (input.pos != input.size)
    {
        // Request decompress in 4k chunks.  This number is arbirary
        constexpr size_t chunkSize = 4096;
        auto buffer = compressionBuf.prepare(chunkSize);
        ZSTD_outBuffer output = {buffer.data(), buffer.size(), 0};
        const size_t ret = ZSTD_decompressStream(dctx, &output, &input);
        if (ZSTD_isError(ret))
        {
            BMCWEB_LOG_ERROR("Decompression Failed with code {}:{}", ret,
                             ZSTD_getErrorName(ret));
            return std::nullopt;
        }
        compressionBuf.commit(output.pos);
    }
    return compressionBuf.cdata();
}

ZstdDecompressor::~ZstdDecompressor()
{
    ZSTD_freeDStream(dctx);
}
