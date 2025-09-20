// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#pragma once

#ifdef HAVE_ZSTD
#include <zstd.h>
#endif

#include <boost/asio/buffer.hpp>
#include <boost/beast/core/flat_buffer.hpp>

#include <optional>

class ZstdCompressor
{
    boost::beast::flat_buffer compressionBuf;

#ifdef HAVE_ZSTD
    ZSTD_CCtx* cctx = nullptr;
#endif

  public:
    ZstdCompressor(const ZstdCompressor&) = delete;
    ZstdCompressor(ZstdCompressor&&) = delete;
    ZstdCompressor& operator=(const ZstdCompressor&) = delete;
    ZstdCompressor& operator=(ZstdCompressor&&) = delete;

    ZstdCompressor() = default;

    // must be called before compress.
    bool init();
    std::optional<boost::asio::const_buffer> compress(
        boost::asio::const_buffer buffIn, bool more);
    ~ZstdCompressor();
};
