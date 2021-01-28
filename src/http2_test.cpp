#include <boost/beast/_experimental/test/stream.hpp>
#include <http2.hpp>

#include <array>

#include "gtest/gtest.h"

std::array<uint8_t, 27> testStreamSettingsBuffer{
    0x00, 0x00, 0x12,       // 18 bytes in length
    0x04,                   // settings payload type
    0x00,                   // no flags
    0x00, 0x00, 0x00, 0x00, // Initial stream identifier
    0x00, 0x03,             // max concurrent streams
    0x00, 0x00, 0x00, 0x64, // 100
    0x00, 0x04,             // Initial window size
    0x40, 0x00, 0x00, 0x00, // 1073741824
    0x00, 0x02,             // enable push
    0x00, 0x00, 0x00, 0x00  // false
};

TEST(HTTP2, accept_stream_test)
{
    boost::asio::io_context io;

    http2::mux_stream<boost::beast::test::stream> myHttp2(io);
    boost::beast::test::stream outputStream(io);

    outputStream.connect(myHttp2.next_layer());

    EXPECT_EQ(
        outputStream.write_some(boost::asio::buffer(
            testStreamSettingsBuffer.data(), testStreamSettingsBuffer.size())),
        testStreamSettingsBuffer.size());

    http2::stream_settings ss{};

    bool callbackCalled = false;
    myHttp2.async_accept(
        ss, [&io, &callbackCalled](boost::system::error_code ec,
                                   const http2::stream_settings& client) {
            callbackCalled = true;
            ASSERT_EQ(ec, boost::system::errc::success);
            EXPECT_EQ(client.enable_push, false);
            EXPECT_EQ(client.initial_window_size, 1073741824);
            EXPECT_EQ(client.max_concurrent_streams, 100);
            EXPECT_EQ(client.table_size, 4096);
            EXPECT_EQ(client.max_header_list_size, std::nullopt);
            io.stop();
        });

    io.run();
    EXPECT_EQ(callbackCalled, true);
}

TEST(HTTP2, settings_parser_defaults)
{
    http2::stream_settings_parser sp(0);
    boost::system::error_code ec;
    ASSERT_EQ(sp.done(), true);
    http2::stream_settings ss = sp.release();
    EXPECT_EQ(ss.table_size, 4096);
    EXPECT_EQ(ss.max_concurrent_streams, std::numeric_limits<uint32_t>::max());
    EXPECT_EQ(ss.initial_window_size, 65535);
    EXPECT_EQ(ss.enable_push, true);
    EXPECT_EQ(ss.max_frame_size, 16384);
    EXPECT_EQ(ss.max_header_list_size, std::nullopt);
}

TEST(HTTP2, settings_parser_payload)
{
    http2::stream_settings_parser sp(testStreamSettingsBuffer[2]);
    boost::system::error_code ec;
    size_t header_length = 9;
    size_t payload_length = testStreamSettingsBuffer.size() - header_length;

    size_t read = sp.put(
        boost::asio::buffer(testStreamSettingsBuffer.data() + header_length,
                            payload_length),
        ec);
    EXPECT_EQ(read, 18);
    ASSERT_EQ(sp.done(), true);
    http2::stream_settings ss = sp.release();
    EXPECT_EQ(ss.table_size, 4096);
    EXPECT_EQ(ss.max_concurrent_streams, 100);
    EXPECT_EQ(ss.initial_window_size, 1073741824);
    EXPECT_EQ(ss.enable_push, false);
    EXPECT_EQ(ss.max_frame_size, 16384);
    EXPECT_EQ(ss.max_header_list_size, std::nullopt);
}

TEST(HTTP2, frame_parser)
{
    std::array<uint8_t, 1> testBuffer{0x01};
    http2::frame_header_parser fp;
    boost::system::error_code ec;
    fp.put(boost::asio::buffer(testBuffer.data(), testBuffer.size()), ec);
}