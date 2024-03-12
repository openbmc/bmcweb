#include <zstd.h>

#include <boost/asio/buffer.hpp>
#include <boost/beast/core/flat_buffer.hpp>

#include <optional>

class ZstdDecompressor
{
    boost::beast::flat_buffer compressionBuf;

    ZSTD_DCtx* dctx;

  public:
    ZstdDecompressor(const ZstdDecompressor&) = delete;
    ZstdDecompressor(ZstdDecompressor&&) = delete;
    ZstdDecompressor& operator=(const ZstdDecompressor&) = delete;
    ZstdDecompressor& operator=(ZstdDecompressor&&) = delete;

    ZstdDecompressor();
    std::optional<boost::asio::const_buffer>
        decompress(boost::asio::const_buffer buffIn);
    ~ZstdDecompressor();
};
