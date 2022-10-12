#include "http_request.hpp"
#include "multipart_parser.hpp"

#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

#include <memory>
#include <string_view>
#include <system_error>
#include <vector>

#include <gtest/gtest.h> // IWYU pragma: keep

// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
// IWYU pragma: no_include "gtest/gtest_pred_impl.h"
// IWYU pragma: no_include <boost/beast/http/impl/fields.hpp>
// IWYU pragma: no_include <boost/intrusive/detail/list_iterator.hpp>
// IWYU pragma: no_include <boost/intrusive/detail/tree_iterator.hpp>

namespace
{
using ::testing::Test;

class MultipartTest : public Test
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
    ParserError rc = parser.parse(reqIn);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.boundary,
              "\r\n-----------------------------d74496d66958873e");
    EXPECT_EQ(parser.mime_fields.size(), 3);

    EXPECT_EQ(parser.mime_fields[0].fields.at("Content-Disposition"),
              "form-data; name=\"Test1\"");
    EXPECT_EQ(parser.mime_fields[0].content,
              "111111111111111111111111112222222222222222222222222222222");

    EXPECT_EQ(parser.mime_fields[1].fields.at("Content-Disposition"),
              "form-data; name=\"Test2\"");
    EXPECT_EQ(parser.mime_fields[1].content,
              "{\r\n-----------------------------d74496d66958873e123456");
    EXPECT_EQ(parser.mime_fields[2].fields.at("Content-Disposition"),
              "form-data; name=\"Test3\"");
    EXPECT_EQ(parser.mime_fields[2].content, "{\r\n--------d74496d6695887}");
}

TEST_F(MultipartTest, TestBadMultipartParser1)
{
    req.set("Content-Type",
            "multipart/form-data; "
            "boundary=---------------------------d74496d66958873e");

    req.body() = "-----------------------------d74496d66958873e\r\n"
                 "Content-Disposition: form-data; name=\"Test1\"\r\n\r\n"
                 "1234567890\r\n"
                 "-----------------------------d74496d66958873e\r-\r\n";

    crow::Request reqIn(req, ec);
    ParserError rc = parser.parse(reqIn);

    EXPECT_EQ(rc, ParserError::ERROR_UNEXPECTED_END_OF_INPUT);
}

TEST_F(MultipartTest, TestBadMultipartParser2)
{
    req.set("Content-Type",
            "multipart/form-data; "
            "boundary=---------------------------d74496d66958873e");

    req.body() = "-----------------------------d74496d66958873e\r\n"
                 "Content-Disposition: form-data; name=\"Test1\"\r\n\r\n"
                 "abcd\r\n"
                 "-----------------------------d74496d66958873e-\r\n";

    crow::Request reqIn(req, ec);
    ParserError rc = parser.parse(reqIn);

    EXPECT_EQ(rc, ParserError::ERROR_UNEXPECTED_END_OF_INPUT);
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
    EXPECT_EQ(parser.parse(reqIn), ParserError::ERROR_BOUNDARY_FORMAT);
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
    EXPECT_EQ(parser.parse(reqIn), ParserError::ERROR_BOUNDARY_CR);
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
    EXPECT_EQ(parser.parse(reqIn), ParserError::ERROR_BOUNDARY_LF);
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
    EXPECT_EQ(parser.parse(reqIn), ParserError::ERROR_BOUNDARY_DATA);
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
    EXPECT_EQ(parser.parse(reqIn), ParserError::ERROR_EMPTY_HEADER);
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
    EXPECT_EQ(parser.parse(reqIn), ParserError::ERROR_HEADER_NAME);
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
    EXPECT_EQ(parser.parse(reqIn), ParserError::ERROR_HEADER_VALUE);
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
    EXPECT_EQ(parser.parse(reqIn), ParserError::ERROR_HEADER_ENDING);
}

TEST_F(MultipartTest, TestGoodMultipartParserMultipleHeaders)
{
    req.set("Content-Type",
            "multipart/form-data; "
            "boundary=---------------------------d74496d66958873e");

    req.body() = "-----------------------------d74496d66958873e\r\n"
                 "Content-Disposition: form-data; name=\"Test1\"\r\n"
                 "Other-Header: value=\"v1\"\r\n"
                 "\r\n"
                 "Data1\r\n"
                 "-----------------------------d74496d66958873e--";

    crow::Request reqIn(req, ec);
    ParserError rc = parser.parse(reqIn);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.boundary,
              "\r\n-----------------------------d74496d66958873e");
    ASSERT_EQ(parser.mime_fields.size(), 1);

    EXPECT_EQ(parser.mime_fields[0].fields.at("Content-Disposition"),
              "form-data; name=\"Test1\"");
    EXPECT_EQ(parser.mime_fields[0].fields.at("Other-Header"), "value=\"v1\"");
    EXPECT_EQ(parser.mime_fields[0].content, "Data1");
}

TEST_F(MultipartTest, TestErrorHeaderWithoutColon)
{
    req.set("Content-Type", "multipart/form-data; "
                            "boundary=--end");

    req.body() = "----end\r\n"
                 "abc\r\n"
                 "\r\n"
                 "Data1\r\n"
                 "----end--\r\n";

    crow::Request reqIn(req, ec);
    EXPECT_EQ(parser.parse(reqIn), ParserError::ERROR_UNEXPECTED_END_OF_HEADER);
}

TEST_F(MultipartTest, TestUnknownHeaderIsCorrectlyParsed)
{
    req.set("Content-Type", "multipart/form-data; "
                            "boundary=--end");

    req.body() =
        "----end\r\n"
        "t-DiPpcccc:cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccgcccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccaaaaaa\r\n"
        "\r\n"
        "Data1\r\n"
        "----end--\r\n";

    crow::Request reqIn(req, ec);
    ParserError rc = parser.parse(reqIn);

    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.boundary, "\r\n----end");
    ASSERT_EQ(parser.mime_fields.size(), 1);

    EXPECT_EQ(
        parser.mime_fields[0].fields.at("t-DiPpcccc"),
        "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccgcccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccaaaaaa");
    EXPECT_EQ(parser.mime_fields[0].content, "Data1");
}

TEST_F(MultipartTest, TestErrorMissingSeparatorBetweenMimeFieldsAndData)
{
    req.set(
        "Content-Type",
        "multipart/form-data; boundary=---------------------------d74496d66958873e");

    req.body() =
        "-----------------------------d74496d66958873e\r\n"
        "t-DiPpcccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccgcccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccaaaaaa\r\n"
        "Data1"
        "-----------------------------d74496d66958873e--";

    crow::Request reqIn(req, ec);
    ParserError rc = parser.parse(reqIn);

    EXPECT_EQ(rc, ParserError::ERROR_UNEXPECTED_END_OF_HEADER);
}

TEST_F(MultipartTest, TestDataWithoutMimeFields)
{
    req.set(
        "Content-Type",
        "multipart/form-data; boundary=---------------------------d74496d66958873e");

    req.body() = "-----------------------------d74496d66958873e\r\n"
                 "\r\n"
                 "Data1\r\n"
                 "-----------------------------d74496d66958873e--";

    crow::Request reqIn(req, ec);
    ParserError rc = parser.parse(reqIn);

    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.boundary,
              "\r\n-----------------------------d74496d66958873e");
    ASSERT_EQ(parser.mime_fields.size(), 1);

    EXPECT_EQ(std::distance(parser.mime_fields[0].fields.begin(),
                            parser.mime_fields[0].fields.end()),
              0);
    EXPECT_EQ(parser.mime_fields[0].content, "Data1");
}

TEST_F(MultipartTest, TestErrorMissingFinalBoundry)
{
    req.set("Content-Type", "multipart/form-data; boundary=--XX");

    req.body() =
        "----XX\r\n"
        "Content-Disposition: form-data; name=\"Test2\"\r\n\r\n"
        "t-DiPpccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccAAAAAAAAAAAAAAABCDz\r\n"
        "\335\r\n\r\n";

    crow::Request reqIn(req, ec);
    ParserError rc = parser.parse(reqIn);

    EXPECT_EQ(rc, ParserError::ERROR_UNEXPECTED_END_OF_INPUT);
}

TEST_F(MultipartTest, TestIgnoreDataAfterFinalBoundary)
{
    req.set("Content-Type", "multipart/form-data; boundary=--XX");

    req.body() = "----XX\r\n"
                 "Content-Disposition: form-data; name=\"Test1\"\r\n\r\n"
                 "Data1\r\n"
                 "----XX--\r\n"
                 "Content-Disposition: form-data; name=\"Test2\"\r\n\r\n"
                 "Data2\r\n"
                 "----XX--\r\n";

    crow::Request reqIn(req, ec);
    ParserError rc = parser.parse(reqIn);

    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.boundary, "\r\n----XX");
    EXPECT_EQ(parser.mime_fields.size(), 1);

    EXPECT_EQ(parser.mime_fields[0].fields.at("Content-Disposition"),
              "form-data; name=\"Test1\"");
    EXPECT_EQ(parser.mime_fields[0].content, "Data1");
}

TEST_F(MultipartTest, TestFinalBoundaryIsCorrectlyRecognized)
{
    req.set("Content-Type", "multipart/form-data; boundary=--XX");

    req.body() = "----XX\r\n"
                 "Content-Disposition: form-data; name=\"Test1\"\r\n\r\n"
                 "Data1\r\n"
                 "----XX-abc-\r\n"
                 "StillData1\r\n"
                 "----XX--\r\n";

    crow::Request reqIn(req, ec);
    ParserError rc = parser.parse(reqIn);

    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.boundary, "\r\n----XX");
    EXPECT_EQ(parser.mime_fields.size(), 1);

    EXPECT_EQ(parser.mime_fields[0].fields.at("Content-Disposition"),
              "form-data; name=\"Test1\"");
    EXPECT_EQ(parser.mime_fields[0].content, "Data1\r\n"
                                             "----XX-abc-\r\n"
                                             "StillData1");
}

} // namespace
