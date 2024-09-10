// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "file_test_utilities.hpp"
#include "http_body.hpp"

#include <boost/beast/core/file_base.hpp>
#include <boost/system/error_code.hpp>

#include <array>
#include <cstddef>
#include <cstdio>
#include <span>
#include <string>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::ElementsAre;

namespace bmcweb
{
namespace
{

TEST(HttpHttpBodyValueType, MoveString)
{
    HttpBody::value_type value("teststring");
    // Move constructor
    HttpBody::value_type value2(std::move(value));
    EXPECT_EQ(value2.encodingType, EncodingType::Raw);
    EXPECT_EQ(value2.str(), "teststring");
    EXPECT_EQ(value2.payloadSize(), 10);
}

TEST(HttpHttpBodyValueType, MoveOperatorString)
{
    HttpBody::value_type value;
    value.str() = "teststring";
    // Move constructor
    HttpBody::value_type value2 = std::move(value);
    EXPECT_EQ(value2.encodingType, EncodingType::Raw);
    EXPECT_EQ(value2.str(), "teststring");
    EXPECT_EQ(value2.payloadSize(), 10);
}

TEST(HttpHttpBodyValueType, copysignl)
{
    HttpBody::value_type value;
    value.str() = "teststring";
    // Move constructor
    HttpBody::value_type value2(value);
    EXPECT_EQ(value2.encodingType, EncodingType::Raw);
    EXPECT_EQ(value2.str(), "teststring");
    EXPECT_EQ(value2.payloadSize(), 10);
}

TEST(HttpHttpBodyValueType, CopyOperatorString)
{
    HttpBody::value_type value;
    value.str() = "teststring";
    // Move constructor
    HttpBody::value_type value2 = value;
    EXPECT_EQ(value2.encodingType, EncodingType::Raw);
    EXPECT_EQ(value2.str(), "teststring");
    EXPECT_EQ(value2.payloadSize(), 10);
}

TEST(HttpHttpBodyValueType, MoveFile)
{
    HttpBody::value_type value(EncodingType::Base64);
    TemporaryFileHandle temporaryFile("teststring");
    boost::system::error_code ec;
    value.open(temporaryFile.stringPath.c_str(), boost::beast::file_mode::read,
               ec);
    ASSERT_FALSE(ec);
    // Move constructor
    HttpBody::value_type value2(std::move(value));
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

TEST(HttpHttpBodyValueType, MoveOperatorFile)
{
    HttpBody::value_type value(EncodingType::Base64);
    TemporaryFileHandle temporaryFile("teststring");
    boost::system::error_code ec;
    value.open(temporaryFile.stringPath.c_str(), boost::beast::file_mode::read,
               ec);
    ASSERT_FALSE(ec);
    // Move constructor
    HttpBody::value_type value2 = std::move(value);
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
    HttpBody::value_type value(EncodingType::Base64);
    TemporaryFileHandle temporaryFile("teststring");
    boost::system::error_code ec;
    value.setFd(fileno(fopen(temporaryFile.stringPath.c_str(), "r")), ec);
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
