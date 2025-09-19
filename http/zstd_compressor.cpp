// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "zstd_compressor.hpp"

#include "logging.hpp"

#ifdef HAVE_ZSTD
#include <zstd.h>
#endif
#include <boost/asio/buffer.hpp>

#include <cstddef>
#include <optional>

#ifdef HAVE_ZSTD

bool ZstdCompressor::init()
{
    if (cctx != nullptr)
    {
        BMCWEB_LOG_ERROR("ZstdCompressor already initialized");
        return false;
    }
    cctx = ZSTD_createCCtx();
    if (cctx == nullptr)
    {
        BMCWEB_LOG_ERROR("Failed to create ZstdCompressor");
        return false;
    }

    // 3 is the default compression level for zstd, but set it explicitly
    // so we can tune later if needed]
    size_t ret = ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, 3);
    if (ZSTD_isError(ret))
    {
        BMCWEB_LOG_ERROR("Failed to set compression level {}:{}", ret,
                         ZSTD_getErrorName(ret));
        return false;
    }
    ret = ZSTD_CCtx_setParameter(cctx, ZSTD_c_checksumFlag, 1);
    if (ZSTD_isError(ret))
    {
        BMCWEB_LOG_ERROR("Failed to set checksum flag {}:{}", ret,
                         ZSTD_getErrorName(ret));
        return false;
    }

    return true;
}
#else
bool ZstdCompressor::init()
{
    BMCWEB_LOG_CRITICAL("ZstdCompressor not compiled in");
    return false;
}
#endif

std::optional<boost::asio::const_buffer> ZstdCompressor::compress(
    [[maybe_unused]] boost::asio::const_buffer buffIn, bool more)
{
#ifdef HAVE_ZSTD
    if (cctx == nullptr)
    {
        BMCWEB_LOG_ERROR("ZstdCompressor not initialized");
        return std::nullopt;
    }
    compressionBuf.clear();
    ZSTD_inBuffer input = {buffIn.data(), buffIn.size(), 0};

    // Note, this loop is prone to compression bombs, decompressing chunks that
    // appear very small, but decompress to be very large, given that they're
    // highly decompressible. This algorithm assumes that at this time, the
    // whole file will fit in ram.
    size_t remaining = buffIn.size();
    while (remaining > 0)
    {
        constexpr size_t frameSize = 4096;
        auto buffer = compressionBuf.prepare(frameSize);
        ZSTD_outBuffer output = {buffer.data(), buffer.size(), 0};
        ZSTD_EndDirective dir = ZSTD_e_end;
        if (more)
        {
            dir = ZSTD_e_continue;
        }
        remaining = ZSTD_compressStream2(cctx, &output, &input, dir);
        if (ZSTD_isError(remaining) > 0)
        {
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

ZstdCompressor::~ZstdCompressor()
{
#ifdef HAVE_ZSTD
    ZSTD_freeCCtx(cctx);
#endif
}
