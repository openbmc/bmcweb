#include <http_utility.hpp>
#include <multipart_parser.hpp>

#include <map>

#include "gmock/gmock.h"

class MultipartTest : public ::testing::Test
{
  public:
    boost::beast::http::request<boost::beast::http::string_body> req{};
    MultipartParser parser;
    std::error_code ec;
};

TEST_F(MultipartTest, TestGoodMultipartParser)
{
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

    crow::Request reqIn(req, ec);
    ASSERT_FALSE(parser.parse(reqIn));

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

TEST_F(MultipartTest, TestErrorBoundaryFormat)
{
    req.set("Content-Type",
            "multipart/form-data; "
            "boundary+=---------------------------d74496d66958873e");

    req.body() = "-----------------------------d74496d66958873e\r\n"
                 "Content-Disposition: form-data; name=\"Test1\"\r\n\r\n"
                 "{\"Key1\": 112233}\r\n"
                 "-----------------------------d74496d66958873e\r\n"
                 "Content-Disposition: form-data; name=\"Test2\"\r\n\r\n"
                 "123456\r\n"
                 "-----------------------------d74496d66958873e--\r\n";

    crow::Request reqIn(req, ec);
    ASSERT_EQ(parser.parse(reqIn).value(),
              static_cast<int>(ParserError::ERROR_BOUNDARY_FORMAT));
}

TEST_F(MultipartTest, TestErrorBoundaryCR)
{
    req.set("Content-Type",
            "multipart/form-data; "
            "boundary=---------------------------d74496d66958873e");

    req.body() = "-----------------------------d74496d66958873e"
                 "Content-Disposition: form-data; name=\"Test1\"\r\n\r"
                 "{\"Key1\": 112233}\r\n"
                 "-----------------------------d74496d66958873e\r\n"
                 "Content-Disposition: form-data; name=\"Test2\"\r\n\r\n"
                 "123456\r\n"
                 "-----------------------------d74496d66958873e--\r\n";

    crow::Request reqIn(req, ec);
    ASSERT_EQ(parser.parse(reqIn).value(),
              static_cast<int>(ParserError::ERROR_BOUNDARY_CR));
}

TEST_F(MultipartTest, TestErrorBoundaryLF)
{
    req.set("Content-Type",
            "multipart/form-data; "
            "boundary=---------------------------d74496d66958873e");

    req.body() = "-----------------------------d74496d66958873e\r"
                 "Content-Disposition: form-data; name=\"Test1\"\r\n\r\n"
                 "{\"Key1\": 112233}\r\n"
                 "-----------------------------d74496d66958873e\r\n"
                 "Content-Disposition: form-data; name=\"Test2\"\r\n\r\n"
                 "123456\r\n"
                 "-----------------------------d74496d66958873e--\r\n";

    crow::Request reqIn(req, ec);
    ASSERT_EQ(parser.parse(reqIn).value(),
              static_cast<int>(ParserError::ERROR_BOUNDARY_LF));
}

TEST_F(MultipartTest, TestErrorBoundaryData)
{
    req.set("Content-Type",
            "multipart/form-data; "
            "boundary=---------------------------d7449sd6d66958873e");

    req.body() = "-----------------------------d74496d66958873e\r\n"
                 "Content-Disposition: form-data; name=\"Test1\"\r\n\r\n"
                 "{\"Key1\": 112233}\r\n"
                 "-----------------------------d74496d66958873e\r\n"
                 "Content-Disposition: form-data; name=\"Test2\"\r\n\r\n"
                 "123456\r\n"
                 "-----------------------------d74496d66958873e--\r\n";

    crow::Request reqIn(req, ec);
    ASSERT_EQ(parser.parse(reqIn).value(),
              static_cast<int>(ParserError::ERROR_BOUNDARY_DATA));
}

TEST_F(MultipartTest, TestErrorEmptyHeader)
{
    req.set("Content-Type",
            "multipart/form-data; "
            "boundary=---------------------------d74496d66958873e");

    req.body() = "-----------------------------d74496d66958873e\r\n"
                 ": form-data; name=\"Test1\"\r\n"
                 "{\"Key1\": 112233}\r\n"
                 "-----------------------------d74496d66958873e\r\n"
                 "Content-Disposition: form-data; name=\"Test2\"\r\n"
                 "123456\r\n"
                 "-----------------------------d74496d66958873e--\r\n";

    crow::Request reqIn(req, ec);
    ASSERT_EQ(parser.parse(reqIn).value(),
              static_cast<int>(ParserError::ERROR_EMPTY_HEADER));
}

TEST_F(MultipartTest, TestErrorHeaderName)
{
    req.set("Content-Type",
            "multipart/form-data; "
            "boundary=---------------------------d74496d66958873e");

    req.body() = "-----------------------------d74496d66958873e\r\n"
                 "Content-!!Disposition: form-data; name=\"Test1\"\r\n"
                 "{\"Key1\": 112233}\r\n"
                 "-----------------------------d74496d66958873e\r\n"
                 "Content-Disposition: form-data; name=\"Test2\"\r\n\r\n"
                 "123456\r\n"
                 "-----------------------------d74496d66958873e--\r\n";

    crow::Request reqIn(req, ec);
    ASSERT_EQ(parser.parse(reqIn).value(),
              static_cast<int>(ParserError::ERROR_HEADER_NAME));
}

TEST_F(MultipartTest, TestErrorHeaderValue)
{
    req.set("Content-Type",
            "multipart/form-data; "
            "boundary=---------------------------d74496d66958873e");

    req.body() = "-----------------------------d74496d66958873e\r\n"
                 "Content-Disposition: form-data; name=\"Test1\"\r"
                 "{\"Key1\": 112233}\r\n"
                 "-----------------------------d74496d66958873e\r\n"
                 "Content-Disposition: form-data; name=\"Test2\"\r\n\r\n"
                 "123456\r\n"
                 "-----------------------------d74496d66958873e--\r\n";

    crow::Request reqIn(req, ec);
    ASSERT_EQ(parser.parse(reqIn).value(),
              static_cast<int>(ParserError::ERROR_HEADER_VALUE));
}

TEST_F(MultipartTest, TestErrorHeaderEnding)
{
    req.set("Content-Type",
            "multipart/form-data; "
            "boundary=---------------------------d74496d66958873e");

    req.body() = "-----------------------------d74496d66958873e\r\n"
                 "Content-Disposition: form-data; name=\"Test1\"\r\n\r"
                 "{\"Key1\": 112233}\r\n"
                 "-----------------------------d74496d66958873e\r\n"
                 "Content-Disposition: form-data; name=\"Test2\"\r\n\r\n"
                 "123456\r\n"
                 "-----------------------------d74496d66958873e--\r\n";

    crow::Request reqIn(req, ec);
    ASSERT_EQ(parser.parse(reqIn).value(),
              static_cast<int>(ParserError::ERROR_HEADER_ENDING));
}
