#include <http_utility.hpp>
#include <multipart_parser.hpp>

#include <map>

#include "gmock/gmock.h"

TEST(MultipartTest, TestGoodMultipartParser)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};

    req.set("Content-Type",
            "multipart/form-data; "
            "boundary=---------------------------d74496d66958873e");

    req.body() = "-----------------------------d74496d66958873e\r\n"
                 "Content-Disposition: form-data; name=\"Test1\"\r\n\r\n"
                 "{\"Key1\": 112233}\r\n"
                 "-----------------------------d74496d66958873e\r\n"
                 "Content-Disposition: form-data; name=\"Test2\"\r\n\r\n"
                 "123456\r\n"
                 "-----------------------------d74496d66958873e--\r\n";

    std::error_code ec;
    MultipartParser parser(req, ec);
    ASSERT_FALSE(ec);

    ASSERT_EQ(parser.boundary,
              "\r\n-----------------------------d74496d66958873e");
    ASSERT_EQ(parser.mime_fields.size(), 2);

    ASSERT_EQ(parser.mime_fields[0].fields.at("Content-Disposition"),
              "form-data; name=\"Test1\"");
    ASSERT_EQ(parser.mime_fields[0].content, "{\"Key1\": 112233}");

    ASSERT_EQ(parser.mime_fields[1].fields.at("Content-Disposition"),
              "form-data; name=\"Test2\"");
    ASSERT_EQ(parser.mime_fields[1].content, "123456");
}

TEST(MultipartTest, TestBadMultipartParser)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};

    req.set("Content-Type",
            "multipart/form-data; "
            "boundary=---------------------------d74496d66958873e");

    req.body() = "-----------------------------d74496d66958873e\r\n"
                 "Content-Disposition: form-data; name=\"Test1\"\r\n"
                 "{\"Key1\": 112233}\r\n"
                 "-----------------------------d74496d66958873e\r\n"
                 "Content-Disposition: form-data; name=\"Test2\"\r\n"
                 "123456\r\n"
                 "-----------------------------d74496d66958873e--\r\n";

    std::error_code ec;
    MultipartParser parser(req, ec);
    ASSERT_EQ(ec.value(), static_cast<int>(ParserError::ERROR_EMPTY_HEADER));
}