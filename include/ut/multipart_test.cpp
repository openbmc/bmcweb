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
                 "111111111111111111111111112222222222222222222222222222222\r\n"
                 "-----------------------------d74496d66958873e\r\n"
                 "Content-Disposition: form-data; name=\"Test2\"\r\n\r\n"
                 "{\r\n-----------------------------d74496d66958873e123456\r\n"
                 "-----------------------------d74496d66958873e\r\n"
                 "Content-Disposition: form-data; name=\"Test3\"\r\n\r\n"
                 "{\r\n--------d74496d6695887}\r\n"
                 "-----------------------------d74496d66958873e--\r\n";

    crow::Request reqIn(req, ec);
    auto rc = parser.parse(reqIn);
    ASSERT_FALSE(rc);

    ASSERT_EQ(parser.boundary,
              "\r\n-----------------------------d74496d66958873e");
    ASSERT_EQ(parser.mime_fields.size(), 3);

    ASSERT_EQ(parser.mime_fields[0].fields.at("Content-Disposition"),
              "form-data; name=\"Test1\"");
    ASSERT_EQ(parser.mime_fields[0].content,
              "111111111111111111111111112222222222222222222222222222222");

    ASSERT_EQ(parser.mime_fields[1].fields.at("Content-Disposition"),
              "form-data; name=\"Test2\"");
    ASSERT_EQ(parser.mime_fields[1].content,
              "{\r\n-----------------------------d74496d66958873e123456");
    ASSERT_EQ(parser.mime_fields[2].fields.at("Content-Disposition"),
              "form-data; name=\"Test3\"");
    ASSERT_EQ(parser.mime_fields[2].content, "{\r\n--------d74496d6695887}");
}

TEST_F(MultipartTest, TestErrorBoundaryFormat)
{
    req.set("Content-Type",
            "multipart/form-data; "
            "boundary+=-----------------------------d74496d66958873e");

    req.body() = "-----------------------------d74496d66958873e\r\n"
                 "Content-Disposition: form-data; name=\"Test1\"\r\n\r\n"
                 "{\"Key1\": 11223333333333333333333333333333333333333333}\r\n"
                 "-----------------------------d74496d66958873e\r\n"
                 "Content-Disposition: form-data; name=\"Test2\"\r\n\r\n"
                 "123456\r\n"
                 "-----------------------------d74496d66958873e--\r\n";

    crow::Request reqIn(req, ec);
    ASSERT_EQ(parser.parse(reqIn),
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
    ASSERT_EQ(parser.parse(reqIn),
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
    ASSERT_EQ(parser.parse(reqIn),
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
    ASSERT_EQ(parser.parse(reqIn),
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
    ASSERT_EQ(parser.parse(reqIn),
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
    ASSERT_EQ(parser.parse(reqIn),
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
    ASSERT_EQ(parser.parse(reqIn),
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
    ASSERT_EQ(parser.parse(reqIn),
              static_cast<int>(ParserError::ERROR_HEADER_ENDING));
}
