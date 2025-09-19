// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#pragma once

#ifdef HAVE_ZSTD
#include <zstd.h>
#endif

#include <boost/beast/core/flat_buffer.hpp>

#include <optional>
#include <span>

namespace bmcweb
{
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
    bool init(size_t sourceSize);
    std::optional<std::span<const uint8_t>> compress(
        std::span<const uint8_t> buffIn, bool more);
    ~ZstdCompressor();
};
} // namespace bmcweb
