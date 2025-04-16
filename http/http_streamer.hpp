#pragma once
#include "http_body.hpp"

#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/message_generator.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/read.hpp>
namespace crow
{
template <typename Adaptor>
struct HttpStreamerImpl :
    std::enable_shared_from_this<HttpStreamerImpl<Adaptor>>
{
    using Parser =
        std::optional<boost::beast::http::request_parser<bmcweb::HttpBody>>;
    // request body limit size set by the BMCWEB_HTTP_BODY_LIMIT option
    static constexpr uint64_t httpReqBodyLimit =
        1024UL * 1024UL * BMCWEB_HTTP_BODY_LIMIT;

    static constexpr uint64_t loggedOutPostBodyLimit = 4096U;

    static constexpr uint32_t httpHeaderLimit = 8192U;
    Parser parser;
    boost::beast::flat_static_buffer<8192> buffer;
    Adaptor& adaptor;
    HttpType httpType;
    HttpStreamerImpl(Adaptor& aAdapter, HttpType type) :
        adaptor(aAdapter), httpType(type)

    {
        parser.emplace();
        parser->header_limit(httpHeaderLimit);
        parser->body_limit(boost::none);
    }
    bool isParserDone() const
    {
        return parser->is_done();
    }
    boost::beast::http::request<bmcweb::HttpBody>& request()
    {
        return parser->get();
    }
    boost::beast::http::request<bmcweb::HttpBody> releaseRequest()
    {
        return parser->release();
    }
    void bodyLimit(uint64_t limit)
    {
        parser->body_limit(limit);
    }
    void doReadHeaders(auto callback)
    {
        if (httpType == HttpType::HTTP)
        {
            boost::beast::http::async_read_header(adaptor.next_layer(), buffer,
                                                  *parser, std::move(callback));
        }
        else
        {
            boost::beast::http::async_read_header(adaptor, buffer, *parser,
                                                  std::move(callback));
        }
    }
    void doRead(auto callback)
    {
        if (httpType == HttpType::HTTP)
        {
            boost::beast::http::async_read_some(adaptor.next_layer(), buffer,
                                                *parser, std::move(callback));
        }
        else
        {
            boost::beast::http::async_read_some(adaptor, buffer, *parser,
                                                std::move(callback));
        }
    }

    boost::beast::flat_static_buffer<8192>& getBuffer()
    {
        return buffer;
    }
    boost::optional<uint64_t> contentLength()
    {
        return parser->content_length();
    }
    auto& base()
    {
        return parser->get().base();
    }
};
} // namespace crow
