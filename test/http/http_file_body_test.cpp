#include "file_test_utilities.hpp"
#include "http_file_body.hpp"

#include <boost/system/error_code.hpp>

#include <array>
#include <span>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::ElementsAre;

namespace bmcweb
{
namespace
{

TEST(HttpFileBodyValueType, MoveString)
{
    FileBody::value_type value("teststring");
    // Move constructor
    FileBody::value_type value2(std::move(value));
    EXPECT_EQ(value2.encodingType, EncodingType::Raw);
    EXPECT_EQ(value2.str(), "teststring");
    EXPECT_EQ(value2.payloadSize(), 10);
}

TEST(HttpFileBodyValueType, MoveOperatorString)
{
    FileBody::value_type value;
    value.str() = "teststring";
    // Move constructor
    FileBody::value_type value2 = std::move(value);
    EXPECT_EQ(value2.encodingType, EncodingType::Raw);
    EXPECT_EQ(value2.str(), "teststring");
    EXPECT_EQ(value2.payloadSize(), 10);
}

TEST(HttpFileBodyValueType, copysignl)
{
    FileBody::value_type value;
    value.str() = "teststring";
    // Move constructor
    FileBody::value_type value2(value);
    EXPECT_EQ(value2.encodingType, EncodingType::Raw);
    EXPECT_EQ(value2.str(), "teststring");
    EXPECT_EQ(value2.payloadSize(), 10);
}

TEST(HttpFileBodyValueType, CopyOperatorString)
{
    FileBody::value_type value;
    value.str() = "teststring";
    // Move constructor
    FileBody::value_type value2 = value;
    EXPECT_EQ(value2.encodingType, EncodingType::Raw);
    EXPECT_EQ(value2.str(), "teststring");
    EXPECT_EQ(value2.payloadSize(), 10);
}

TEST(HttpFileBodyValueType, MoveFile)
{
    FileBody::value_type value(EncodingType::Base64);
    std::string filepath = makeFile("teststring");
    boost::system::error_code ec;
    value.open(filepath.c_str(), boost::beast::file_mode::read, ec);
    ASSERT_FALSE(ec);
    // Move constructor
    FileBody::value_type value2(std::move(value));
    std::array<char, 11> buffer{};
    size_t out = value2.file().read(buffer.data(), buffer.size(), ec);
    ASSERT_FALSE(ec);
    EXPECT_EQ(value2.encodingType, EncodingType::Base64);

    EXPECT_THAT(std::span(buffer.data(), out),
                ElementsAre('t', 'e', 's', 't', 's', 't', 'r', 'i', 'n', 'g'));

    EXPECT_THAT(buffer, ElementsAre('t', 'e', 's', 't', 's', 't', 'r', 'i', 'n',
                                    'g', '\0'));

    EXPECT_EQ(value2.payloadSize(), 16);
}

TEST(HttpFileBodyValueType, MoveOperatorFile)
{
    FileBody::value_type value(EncodingType::Base64);
    std::string filepath = makeFile("teststring");
    boost::system::error_code ec;
    value.open(filepath.c_str(), boost::beast::file_mode::read, ec);
    ASSERT_FALSE(ec);
    // Move constructor
    FileBody::value_type value2 = std::move(value);
    std::array<char, 11> buffer{};
    size_t out = value2.file().read(buffer.data(), buffer.size(), ec);
    ASSERT_FALSE(ec);
    EXPECT_EQ(value2.encodingType, EncodingType::Base64);

    EXPECT_THAT(std::span(buffer.data(), out),
                ElementsAre('t', 'e', 's', 't', 's', 't', 'r', 'i', 'n', 'g'));
    EXPECT_THAT(buffer, ElementsAre('t', 'e', 's', 't', 's', 't', 'r', 'i', 'n',
                                    'g', '\0'));

    EXPECT_EQ(value2.payloadSize(), 16);
}

TEST(HttpFileBodyValueType, SetFd)
{
    FileBody::value_type value(EncodingType::Base64);
    std::string filepath = makeFile("teststring");

    boost::system::error_code ec;
    value.setFd(fileno(fopen(filepath.c_str(), "r")), ec);
    ASSERT_FALSE(ec);

    std::array<char, 4096> buffer{};

    size_t out = value.file().read(buffer.data(), buffer.size(), ec);
    ASSERT_FALSE(ec);

    EXPECT_THAT(std::span(buffer.data(), out),
                ElementsAre('t', 'e', 's', 't', 's', 't', 'r', 'i', 'n', 'g'));
    EXPECT_EQ(value.payloadSize(), 16);
}

} // namespace
} // namespace bmcweb
