// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#pragma once

#ifdef HAVE_ZSTD
#include <zstd.h>
#endif

#include <boost/asio/buffer.hpp>
#include <boost/beast/core/flat_buffer.hpp>

#include <optional>

class ZstdDecompressor
{
    boost::beast::flat_buffer compressionBuf;

#ifdef HAVE_ZSTD
    ZSTD_DCtx* dctx;
#endif

  public:
    ZstdDecompressor(const ZstdDecompressor&) = delete;
    ZstdDecompressor(ZstdDecompressor&&) = delete;
    ZstdDecompressor& operator=(const ZstdDecompressor&) = delete;
    ZstdDecompressor& operator=(ZstdDecompressor&&) = delete;

    ZstdDecompressor();
    std::optional<boost::asio::const_buffer> decompress(
        boost::asio::const_buffer buffIn);
    ~ZstdDecompressor();
};
