#include "credential_pipe.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/beast/core/file_posix.hpp>

#include <string>

#include <gmock/gmock.h>

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
    boost::beast::file_posix file;
    {
        boost::asio::io_context io;
        CredentialsPipe pipe(io);
        file.native_handle(dup(pipe.fd()));
        ASSERT_GT(file.native_handle(), 0);
        pipe.asyncWrite("username", "password",
                        std::bind_front(handler, std::ref(io)));
        io.run();
    }
    std::array<char, 18> buff;
    boost::system::error_code ec;
    size_t r = file.read(buff.data(), buff.size(), ec);
    ASSERT_FALSE(ec);
    ASSERT_EQ(r, 18);

    EXPECT_THAT(buff,
                ElementsAre('u', 's', 'e', 'r', 'n', 'a', 'm', 'e', '\0', 'p',
                            'a', 's', 's', 'w', 'o', 'r', 'd', '\0'));
}
