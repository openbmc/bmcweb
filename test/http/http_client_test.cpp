// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "http/http_body.hpp"
#include "http/http_client.hpp"
#include "http_response.hpp"
#include "ssl_key_handler.hpp"

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include <boost/asio/buffer.hpp>
#include <boost/asio/connect_pipe.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/impl/connect_pipe.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/readable_pipe.hpp>
#include <boost/asio/writable_pipe.hpp>
#include <boost/asio/write.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/url.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include "gtest/gtest.h"

namespace crow
{
namespace
{

using HttpResponse = boost::beast::http::response<bmcweb::HttpBody>;

HttpResponse makeResponse(boost::beast::http::status status,
                          std::string_view contentType,
                          std::string_view contentLength)
{
    HttpResponse res;
    res.result(status);
    if (!contentType.empty())
    {
        res.set(boost::beast::http::field::content_type, contentType);
    }
    if (!contentLength.empty())
    {
        res.set(boost::beast::http::field::content_length, contentLength);
    }
    return res;
}

// parseStreamingContentLength tests

TEST(ParseStreamingContentLength, PresentAndValid)
{
    HttpResponse res =
        makeResponse(boost::beast::http::status::ok, "", "1234567");
    EXPECT_EQ(parseStreamingContentLength(res), 1234567U);
}

TEST(ParseStreamingContentLength, Missing)
{
    HttpResponse res;
    res.result(boost::beast::http::status::ok);
    EXPECT_EQ(parseStreamingContentLength(res), 0U);
}

TEST(ParseStreamingContentLength, NonNumericReturnsZero)
{
    HttpResponse res = makeResponse(boost::beast::http::status::ok, "", "abc");
    EXPECT_EQ(parseStreamingContentLength(res), 0U);
}

TEST(ParseStreamingContentLength, ZeroReturnsZero)
{
    HttpResponse res = makeResponse(boost::beast::http::status::ok, "", "0");
    EXPECT_EQ(parseStreamingContentLength(res), 0U);
}

// shouldStreamResponse tests

TEST(ShouldStreamResponse, BinaryResponseWithContentStreams)
{
    HttpResponse res = makeResponse(boost::beast::http::status::ok,
                                    "application/octet-stream", "1000000");
    EXPECT_TRUE(shouldStreamResponse(res, 1000000, false));
}

TEST(ShouldStreamResponse, JsonResponseIsBuffered)
{
    HttpResponse res = makeResponse(boost::beast::http::status::ok,
                                    "application/json", "1000");
    EXPECT_FALSE(shouldStreamResponse(res, 1000, false));
}

TEST(ShouldStreamResponse, JsonWithCharsetIsBuffered)
{
    HttpResponse res = makeResponse(boost::beast::http::status::ok,
                                    "application/json;charset=utf-8", "1000");
    EXPECT_FALSE(shouldStreamResponse(res, 1000, false));
}

TEST(ShouldStreamResponse, ZeroContentLengthIsBuffered)
{
    HttpResponse res = makeResponse(boost::beast::http::status::ok,
                                    "application/octet-stream", "0");
    EXPECT_FALSE(shouldStreamResponse(res, 0, false));
}

TEST(ShouldStreamResponse, NoContentStatusIsBuffered)
{
    HttpResponse res;
    res.result(boost::beast::http::status::no_content);
    EXPECT_FALSE(shouldStreamResponse(res, 0, false));
}

TEST(ShouldStreamResponse, InvalidResponseIsBuffered)
{
    HttpResponse res =
        makeResponse(boost::beast::http::status::internal_server_error,
                     "application/octet-stream", "1000");
    EXPECT_FALSE(shouldStreamResponse(res, 1000, /*responseIsInvalid=*/true));
}

TEST(ShouldStreamResponse, NoContentTypeWithBodyStreams)
{
    HttpResponse res =
        makeResponse(boost::beast::http::status::ok, "", "5000000");
    EXPECT_TRUE(shouldStreamResponse(res, 5000000, false));
}

// BUG-5: closing write pipe without flush signals short EOF to reader

TEST(StreamingPipeBug5, ClosingWritePipeWithoutFlush_SignalsShortEofToReader)
{
    boost::asio::io_context ioc;
    boost::asio::writable_pipe writePipe(ioc);
    boost::asio::readable_pipe readPipe(ioc);
    boost::system::error_code pipeEc;
    boost::asio::connect_pipe(readPipe, writePipe, pipeEc);
    ASSERT_FALSE(pipeEc);

    const std::string partialData(128, 'X');
    boost::asio::write(writePipe, boost::asio::buffer(partialData), pipeEc);
    ASSERT_FALSE(pipeEc);

    writePipe.close();

    std::string buf(1024, '\0');
    boost::system::error_code readEc;
    size_t n = boost::asio::read(readPipe, boost::asio::buffer(buf), readEc);

    EXPECT_EQ(readEc, boost::asio::error::eof);
    EXPECT_EQ(n, partialData.size());
}

// BUG-4: full buffer produces zero readLimit, documenting the busy-loop guard
// arithmetic

TEST(StreamingReadLimitBug4, FullBufferProducesZeroReadLimit)
{
    constexpr size_t bufferMaxSize = httpReadBufferSize;
    const size_t bufferUsed = bufferMaxSize; // completely occupied
    const size_t remaining = 1024UL * 1024;  // data still to receive

    const size_t readLimit = std::min(bufferMaxSize - bufferUsed, remaining);

    EXPECT_EQ(readLimit, 0U);
}

// BUG-1: malformed header exhausts retries sequentially and fires callback
// exactly once

TEST(HttpClientBug1, MalformedHmcHeader_CallbackOnceAtRetryExhaustion)
{
    boost::asio::io_context ioc;

    // --- Fake HMC: for every TCP connection send garbage then close ---
    boost::asio::ip::tcp::acceptor acceptor(
        ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 0));
    int connectionCount = 0;

    std::function<void()> doAccept = [&]() {
        auto sock = std::make_shared<boost::asio::ip::tcp::socket>(ioc);
        acceptor.async_accept(*sock, [&, sock](boost::system::error_code ec) {
            if (ec)
            {
                return;
            }
            ++connectionCount;
            auto garbage = std::make_shared<std::string>("GARBAGE\r\n\r\n");
            boost::asio::async_write(
                *sock, boost::asio::buffer(*garbage),
                [sock, garbage](boost::system::error_code, std::size_t) {
                    sock->close();
                });
            doAccept();
        });
    };
    doAccept();

    uint16_t port = acceptor.local_endpoint().port();
    boost::urls::url destUrl(
        "http://127.0.0.1:" + std::to_string(port) + "/fdr");

    auto policy = std::make_shared<ConnectionPolicy>();
    policy->maxRetryAttempts = 1;
    policy->retryIntervalSecs = std::chrono::seconds(0);
    policy->retryPolicyAction = "TerminateAfterRetries";

    HttpClient client(ioc, policy);

    int callbackCount = 0;
    boost::beast::http::status callbackStatus = boost::beast::http::status::ok;

    boost::beast::http::fields headers;
    client.sendDataWithCallback(
        "", destUrl, ensuressl::VerifyCertificate::NoVerify, headers,
        boost::beast::http::verb::get, [&](Response& res) {
            ++callbackCount;
            callbackStatus = res.result();
            acceptor.close();
            ioc.stop();
        });

    ioc.run();

    EXPECT_EQ(callbackCount, 1);
    EXPECT_EQ(callbackStatus, boost::beast::http::status::bad_gateway);
    EXPECT_EQ(connectionCount, 2); // initial + 1 retry
}

// Helper: dup the streaming pipe fd and clear O_NONBLOCK for blocking reads
// after ioc.run().
int dupStreamFd(Response& res)
{
    int raw = res.response.body().file().native_handle();
    if (raw < 0)
    {
        return -1;
    }
    int fd = ::dup(raw);
    if (fd < 0)
    {
        return -1;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    int flags = ::fcntl(fd, F_GETFL);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    ::fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
    return fd;
}

// Helper: read a blocking fd to string until EOF.
std::string drainFd(int fd)
{
    std::string out;
    std::array<char, 4096> buf{};
    for (;;)
    {
        ssize_t n = ::read(fd, buf.data(), buf.size());
        if (n <= 0)
        {
            break;
        }
        out.append(buf.data(), static_cast<size_t>(n));
    }
    return out;
}

// Helper: minimal HTTP/1.1 200 octet-stream response header.
std::string makeHmcHeader(size_t contentLength)
{
    return "HTTP/1.1 200 OK\r\n"
           "Content-Type: application/octet-stream\r\n"
           "Content-Length: " +
           std::to_string(contentLength) + "\r\n\r\n";
}

// TCP-drop mid-stream: HMC closes socket early; reader gets short EOF, not
// silent corruption.

TEST(HmcTruncation, TcpDropMidStream_PipeGivesShortEof)
{
    boost::asio::io_context ioc;

    constexpr size_t sentSize = httpReadBufferSize;
    constexpr size_t claimedSize = sentSize * 2;

    boost::asio::ip::tcp::acceptor acceptor(
        ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 0));

    auto doHmcSession = [&]() {
        auto sock = std::make_shared<boost::asio::ip::tcp::socket>(ioc);
        acceptor.async_accept(*sock, [&, sock](boost::system::error_code ec) {
            if (ec)
            {
                return;
            }
            auto hdr =
                std::make_shared<std::string>(makeHmcHeader(claimedSize));
            boost::asio::async_write(
                *sock, boost::asio::buffer(*hdr),
                [&, sock, hdr](boost::system::error_code, std::size_t) {
                    auto body = std::make_shared<std::string>(sentSize, 'X');
                    boost::asio::async_write(
                        *sock, boost::asio::buffer(*body),
                        [&acceptor, sock,
                         body](boost::system::error_code, std::size_t) {
                            sock->close();    // TCP drop mid-transfer
                            acceptor.close(); // no further connections
                        });
                });
        });
    };
    doHmcSession();

    uint16_t port = acceptor.local_endpoint().port();
    boost::urls::url destUrl(
        "http://127.0.0.1:" + std::to_string(port) + "/fdr");

    auto policy = std::make_shared<ConnectionPolicy>();
    policy->maxRetryAttempts = 0;
    policy->retryIntervalSecs = std::chrono::seconds(0);
    policy->retryPolicyAction = "TerminateAfterRetries";

    HttpClient client(ioc, policy);
    int capturedFd = -1;

    boost::beast::http::fields hdrs;
    client.sendDataWithCallback(
        "", destUrl, ensuressl::VerifyCertificate::NoVerify, hdrs,
        boost::beast::http::verb::get,
        [&](Response& res) { capturedFd = dupStreamFd(res); });

    ioc.run();

    ASSERT_GE(capturedFd, 0) << "header callback did not fire";

    std::string received = drainFd(capturedFd);
    ::close(capturedFd);

    EXPECT_EQ(received.size(), sentSize);    // all sent bytes arrived
    EXPECT_LT(received.size(), claimedSize); // short of Content-Length
}

TEST(HmcTruncation, DISABLED_StallTimer120s_Integration)
{
    boost::asio::io_context ioc;

    constexpr size_t sentSize = 1024UL * 1024; // 1 MiB
    constexpr size_t claimedSize = sentSize * 2;

    boost::asio::ip::tcp::acceptor acceptor(
        ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 0));

    // Keep stallSock alive so the TCP connection stays open after body bytes
    // are written.
    auto stallSock = std::make_shared<boost::asio::ip::tcp::socket>(ioc);

    acceptor.async_accept(*stallSock, [&, stallSock](
                                          boost::system::error_code ec) {
        if (ec)
        {
            return;
        }
        auto hdr = std::make_shared<std::string>(makeHmcHeader(claimedSize));
        boost::asio::async_write(
            *stallSock, boost::asio::buffer(*hdr),
            [&, stallSock, hdr](boost::system::error_code, std::size_t) {
                auto body = std::make_shared<std::string>(sentSize, 'X');
                boost::asio::async_write(
                    *stallSock, boost::asio::buffer(*body),
                    [&acceptor, stallSock,
                     body](boost::system::error_code, std::size_t) {
                        acceptor.close();
                    });
            });
    });

    uint16_t port = acceptor.local_endpoint().port();
    boost::urls::url destUrl(
        "http://127.0.0.1:" + std::to_string(port) + "/fdr");

    auto policy = std::make_shared<ConnectionPolicy>();
    policy->maxRetryAttempts = 0;
    policy->retryIntervalSecs = std::chrono::seconds(0);
    policy->retryPolicyAction = "TerminateAfterRetries";

    HttpClient client(ioc, policy);
    int capturedFd = -1;

    boost::beast::http::fields hdrs;
    client.sendDataWithCallback(
        "", destUrl, ensuressl::VerifyCertificate::NoVerify, hdrs,
        boost::beast::http::verb::get,
        [&](Response& res) { capturedFd = dupStreamFd(res); });

    ioc.run_for(
        std::chrono::seconds(135)); // outlast the 120 s chunk-stall timer

    stallSock->close();

    ASSERT_GE(capturedFd, 0) << "header callback did not fire";

    std::string received = drainFd(capturedFd);
    ::close(capturedFd);

    EXPECT_LT(received.size(), claimedSize);
}

// HMC 404 JSON: shouldStreamResponse returns false; body is buffered, no relay
// pipe opened.

TEST(HmcBufferedPath, Http404Json_RoutedToBufferedPath_NoPipe)
{
    boost::asio::io_context ioc;

    const std::string jsonBody =
        R"({"error":{"code":"Base.1.0.ResourceNotFound","message":"The resource was not found"}})";

    boost::asio::ip::tcp::acceptor acceptor(
        ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 0));

    auto sock = std::make_shared<boost::asio::ip::tcp::socket>(ioc);
    acceptor.async_accept(*sock, [&, sock](boost::system::error_code ec) {
        if (ec)
        {
            return;
        }
        const std::string response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: " +
            std::to_string(jsonBody.size()) + "\r\n\r\n" + jsonBody;
        auto buf = std::make_shared<std::string>(response);
        boost::asio::async_write(
            *sock, boost::asio::buffer(*buf),
            [&acceptor, sock, buf](boost::system::error_code, std::size_t) {
                sock->close();
                acceptor.close();
            });
    });

    uint16_t port = acceptor.local_endpoint().port();
    boost::urls::url destUrl(
        "http://127.0.0.1:" + std::to_string(port) + "/fdr");

    auto policy = std::make_shared<ConnectionPolicy>();
    policy->maxRetryAttempts = 0;
    policy->retryIntervalSecs = std::chrono::seconds(0);
    policy->retryPolicyAction = "TerminateAfterRetries";
    // Pass all status codes through so 404 reaches the caller rather than
    // retrying to 502.
    policy->invalidResp = [](unsigned int /*respCode*/) {
        return boost::system::errc::make_error_code(
            boost::system::errc::success);
    };

    HttpClient client(ioc, policy);

    int callbackCount = 0;
    boost::beast::http::status callbackStatus = boost::beast::http::status::ok;
    std::string callbackBody;
    bool pipeOpened = false;

    boost::beast::http::fields hdrs;
    client.sendDataWithCallback(
        "", destUrl, ensuressl::VerifyCertificate::NoVerify, hdrs,
        boost::beast::http::verb::get, [&](Response& res) {
            ++callbackCount;
            callbackStatus = res.result();
            callbackBody = res.response.body().str();
            pipeOpened = res.response.body().file().is_open();
            acceptor.close();
            ioc.stop();
        });

    ioc.run();

    EXPECT_EQ(callbackCount, 1);
    EXPECT_EQ(callbackStatus, boost::beast::http::status::not_found);
    EXPECT_NE(callbackBody.find("ResourceNotFound"), std::string::npos);
    EXPECT_FALSE(pipeOpened);
}

} // namespace
} // namespace crow
