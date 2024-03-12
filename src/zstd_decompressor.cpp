// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "zstd_decompressor.hpp"

#include "logging.hpp"

#ifdef HAVE_ZSTD
#include <zstd.h>
#endif
#include <boost/asio/buffer.hpp>

#include <cstddef>
#include <optional>

ZstdDecompressor::ZstdDecompressor()
{
#ifdef HAVE_ZSTD
    dctx = ZSTD_createDStream();
    ZSTD_initDStream(dctx);
#endif
}

std::optional<boost::asio::const_buffer> ZstdDecompressor::decompress(
    [[maybe_unused]] boost::asio::const_buffer buffIn)
{
#ifdef HAVE_ZSTD
    compressionBuf.clear();
    ZSTD_inBuffer input = {buffIn.data(), buffIn.size(), 0};

    // Note, this loop is prone to compression bombs, decompressing chunks that
    // appear very small, but decompress to be very large, given that they're
    // highly decompressible. This algorithm assumes that at this time, the
    // whole file will fit in ram.
    while (input.pos != input.size)
    {
        constexpr size_t frameSize = 4096;
        auto buffer = compressionBuf.prepare(frameSize);
        ZSTD_outBuffer output = {buffer.data(), buffer.size(), 0};
        const size_t ret = ZSTD_decompressStream(dctx, &output, &input);
        if (ZSTD_isError(ret) != 0)
        {
            BMCWEB_LOG_ERROR("Decompression Failed with code {}:{}", ret,
                             ZSTD_getErrorName(ret));
            return std::nullopt;
        }
        compressionBuf.commit(output.pos);
    }
    return compressionBuf.cdata();
#else
    BMCWEB_LOG_CRITICAL("Attempt to decompress, but libzstd not enabled");

    return std::nullopt;
#endif
}

ZstdDecompressor::~ZstdDecompressor()
{
#ifdef HAVE_ZSTD
    ZSTD_freeDStream(dctx);
#endif
}
