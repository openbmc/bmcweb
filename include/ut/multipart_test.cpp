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
    crow::Request reqIn(req, ec);
    MultipartParser parser;
    parser.parse(reqIn, ec);
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

TEST(MultipartTest, TestErrorBoundaryFormat)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};

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

    std::error_code ec;
    crow::Request reqIn(req, ec);
    MultipartParser parser;
    parser.parse(reqIn, ec);
    ASSERT_EQ(ec.value(), static_cast<int>(ParserError::ERROR_BOUNDARY_FORMAT));
}

TEST(MultipartTest, TestErrorBoundaryCR)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};

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

    std::error_code ec;
    crow::Request reqIn(req, ec);
    MultipartParser parser;
    parser.parse(reqIn, ec);
    ASSERT_EQ(ec.value(), static_cast<int>(ParserError::ERROR_BOUNDARY_CR));
}

TEST(MultipartTest, TestErrorBoundaryLF)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};

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

    std::error_code ec;
    crow::Request reqIn(req, ec);
    MultipartParser parser;
    parser.parse(reqIn, ec);
    ASSERT_EQ(ec.value(), static_cast<int>(ParserError::ERROR_BOUNDARY_LF));
}

TEST(MultipartTest, TestErrorBoundaryData)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};

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

    std::error_code ec;
    crow::Request reqIn(req, ec);
    MultipartParser parser;
    parser.parse(reqIn, ec);
    ASSERT_EQ(ec.value(), static_cast<int>(ParserError::ERROR_BOUNDARY_DATA));
}

TEST(MultipartTest, TestErrorEmptyHeader)
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
    crow::Request reqIn(req, ec);
    MultipartParser parser;
    parser.parse(reqIn, ec);
    ASSERT_EQ(ec.value(), static_cast<int>(ParserError::ERROR_EMPTY_HEADER));
}

TEST(MultipartTest, TestErrorHeaderValue)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};

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

    std::error_code ec;
    crow::Request reqIn(req, ec);
    MultipartParser parser;
    parser.parse(reqIn, ec);
    ASSERT_EQ(ec.value(), static_cast<int>(ParserError::ERROR_HEADER_VALUE));
}

TEST(MultipartTest, TestErrorHeaderEnding)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};

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

    std::error_code ec;
    crow::Request reqIn(req, ec);
    MultipartParser parser;
    parser.parse(reqIn, ec);
    ASSERT_EQ(ec.value(), static_cast<int>(ParserError::ERROR_HEADER_ENDING));
}
