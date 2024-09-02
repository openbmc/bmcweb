#include "credential_pipe.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/readable_pipe.hpp>
#include <boost/system/error_code.hpp>

#include <array>
#include <cstddef>
#include <functional>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::ElementsAre;

static void handler(boost::asio::io_context& io,
                    const boost::system::error_code& ec, size_t sent)
{
    io.stop();
    EXPECT_FALSE(ec);
    EXPECT_EQ(sent, 18);
}

TEST(CredentialsPipe, BasicSend)
{
    boost::asio::io_context io;
    boost::asio::readable_pipe testPipe(io);
    {
        CredentialsPipe pipe(io);
        testPipe = boost::asio::readable_pipe(io, pipe.releaseFd());
        ASSERT_GT(testPipe.native_handle(), 0);
        pipe.asyncWrite("username", "password",
                        std::bind_front(handler, std::ref(io)));
    }
    io.run();
    std::array<char, 18> buff{};
    boost::system::error_code ec;
    boost::asio::read(testPipe, boost::asio::buffer(buff), ec);
    ASSERT_FALSE(ec);

    EXPECT_THAT(buff,
                ElementsAre('u', 's', 'e', 'r', 'n', 'a', 'm', 'e', '\0', 'p',
                            'a', 's', 's', 'w', 'o', 'r', 'd', '\0'));
}
