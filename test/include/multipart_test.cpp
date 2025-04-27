// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "http_request.hpp"
#include "multipart_parser.hpp"

#include <iterator>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

namespace
{
using ::testing::Test;

class MultipartTest : public Test
{
  public:
    MultipartParser parser;
    std::error_code ec;
};

TEST_F(MultipartTest, TestGoodMultipartParser)
{
    std::string_view body =
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test1\"\r\n\r\n"
        "111111111111111111111111112222222222222222222222222222222\r\n"
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test2\"\r\n\r\n"
        "{\r\n-----------------------------d74496d66958873e123456\r\n"
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test3\"\r\n\r\n"
        "{\r\n--------d74496d6695887}\r\n"
        "-----------------------------d74496d66958873e--\r\n";

    crow::Request reqIn(body, ec);

    ParserError rc = parser.parse(
        "multipart/form-data; boundary=---------------------------d74496d66958873e",
        body);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.boundary,
              "\r\n-----------------------------d74496d66958873e");
    EXPECT_EQ(parser.mime_fields.size(), 3);

    EXPECT_EQ(parser.mime_fields[0].fields.at("Content-Disposition"),
              "form-data; name=\"Test1\"");
    const std::string& content0 =
        std::get<std::string>(parser.mime_fields[0].content);
    EXPECT_EQ(content0,
              "111111111111111111111111112222222222222222222222222222222");

    EXPECT_EQ(parser.mime_fields[1].fields.at("Content-Disposition"),
              "form-data; name=\"Test2\"");
    const std::string& content1 =
        std::get<std::string>(parser.mime_fields[1].content);
    EXPECT_EQ(content1,
              "{\r\n-----------------------------d74496d66958873e123456");
    EXPECT_EQ(parser.mime_fields[2].fields.at("Content-Disposition"),
              "form-data; name=\"Test3\"");
    const std::string& content2 =
        std::get<std::string>(parser.mime_fields[2].content);
    EXPECT_EQ(content2, "{\r\n--------d74496d6695887}");
}

TEST_F(MultipartTest, TestBadMultipartParser1)
{
    std::string_view body =
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test1\"\r\n\r\n"
        "1234567890\r\n"
        "-----------------------------d74496d66958873e\r-\r\n";

    crow::Request reqIn(body, ec);

    ParserError rc = parser.parse(
        "multipart/form-data; boundary=---------------------------d74496d66958873e",
        body);

    EXPECT_EQ(rc, ParserError::ERROR_UNEXPECTED_END_OF_INPUT);
}

TEST_F(MultipartTest, TestBadMultipartParser2)
{
    std::string_view body =
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test1\"\r\n\r\n"
        "abcd\r\n"
        "-----------------------------d74496d66958873e-\r\n";
    crow::Request reqIn(body, ec);

    ParserError rc = parser.parse(
        "multipart/form-data; boundary=---------------------------d74496d66958873e",
        body);

    EXPECT_EQ(rc, ParserError::ERROR_UNEXPECTED_END_OF_INPUT);
}

TEST_F(MultipartTest, TestErrorBoundaryFormat)
{
    std::string_view body =
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test1\"\r\n\r\n"
        "{\"Key1\": 11223333333333333333333333333333333333333333}\r\n"
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test2\"\r\n\r\n"
        "123456\r\n"
        "-----------------------------d74496d66958873e--\r\n";

    crow::Request reqIn(body, ec);

    EXPECT_EQ(
        parser.parse(
            "multipart/form-data; boundary+=-----------------------------d74496d66958873e",
            body),
        ParserError::ERROR_BOUNDARY_FORMAT);
}

TEST_F(MultipartTest, TestErrorBoundaryCR)
{
    std::string_view body =
        "-----------------------------d74496d66958873e"
        "Content-Disposition: form-data; name=\"Test1\"\r\n\r"
        "{\"Key1\": 112233}\r\n"
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test2\"\r\n\r\n"
        "123456\r\n"
        "-----------------------------d74496d66958873e--\r\n";
    crow::Request reqIn(body, ec);

    EXPECT_EQ(
        parser.parse(
            "multipart/form-data; boundary=---------------------------d74496d66958873e",
            body),
        ParserError::ERROR_BOUNDARY_CR);
}

TEST_F(MultipartTest, TestErrorBoundaryLF)
{
    std::string_view body =
        "-----------------------------d74496d66958873e\r"
        "Content-Disposition: form-data; name=\"Test1\"\r\n\r\n"
        "{\"Key1\": 112233}\r\n"
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test2\"\r\n\r\n"
        "123456\r\n"
        "-----------------------------d74496d66958873e--\r\n";

    crow::Request reqIn(body, ec);

    EXPECT_EQ(
        parser.parse(
            "multipart/form-data; boundary=---------------------------d74496d66958873e",
            body),
        ParserError::ERROR_BOUNDARY_LF);
}

TEST_F(MultipartTest, TestErrorBoundaryData)
{
    std::string_view body =
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test1\"\r\n\r\n"
        "{\"Key1\": 112233}\r\n"
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test2\"\r\n\r\n"
        "123456\r\n"
        "-----------------------------d74496d66958873e--\r\n";

    crow::Request reqIn(body, ec);

    EXPECT_EQ(
        parser.parse(
            "multipart/form-data; boundary=---------------------------d7449sd6d66958873e",
            body),
        ParserError::ERROR_BOUNDARY_DATA);
}

TEST_F(MultipartTest, TestErrorEmptyHeader)
{
    std::string_view body =
        "-----------------------------d74496d66958873e\r\n"
        ": form-data; name=\"Test1\"\r\n"
        "{\"Key1\": 112233}\r\n"
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test2\"\r\n"
        "123456\r\n"
        "-----------------------------d74496d66958873e--\r\n";
    crow::Request reqIn(body, ec);

    EXPECT_EQ(
        parser.parse(
            "multipart/form-data; boundary=---------------------------d74496d66958873e",
            body),
        ParserError::ERROR_EMPTY_HEADER);
}

TEST_F(MultipartTest, TestErrorHeaderName)
{
    std::string_view body =
        "-----------------------------d74496d66958873e\r\n"
        "Content-!!Disposition: form-data; name=\"Test1\"\r\n"
        "{\"Key1\": 112233}\r\n"
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test2\"\r\n\r\n"
        "123456\r\n"
        "-----------------------------d74496d66958873e--\r\n";
    crow::Request reqIn(body, ec);

    EXPECT_EQ(
        parser.parse(
            "multipart/form-data; boundary=---------------------------d74496d66958873e",
            body),
        ParserError::ERROR_HEADER_NAME);
}

TEST_F(MultipartTest, TestErrorHeaderValue)
{
    std::string_view body =
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test1\"\r"
        "{\"Key1\": 112233}\r\n"
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test2\"\r\n\r\n"
        "123456\r\n"
        "-----------------------------d74496d66958873e--\r\n";

    crow::Request reqIn(body, ec);

    EXPECT_EQ(
        parser.parse(
            "multipart/form-data; boundary=---------------------------d74496d66958873e",
            body),
        ParserError::ERROR_HEADER_VALUE);
}

TEST_F(MultipartTest, TestErrorHeaderEnding)
{
    std::string_view body =
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test1\"\r\n\r"
        "{\"Key1\": 112233}\r\n"
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test2\"\r\n\r\n"
        "123456\r\n"
        "-----------------------------d74496d66958873e--\r\n";

    crow::Request reqIn(body, ec);

    EXPECT_EQ(
        parser.parse(
            "multipart/form-data; boundary=---------------------------d74496d66958873e",
            body),
        ParserError::ERROR_HEADER_ENDING);
}

TEST_F(MultipartTest, TestGoodMultipartParserMultipleHeaders)
{
    std::string_view body =
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test1\"\r\n"
        "Other-Header: value=\"v1\"\r\n"
        "\r\n"
        "Data1\r\n"
        "-----------------------------d74496d66958873e--";

    crow::Request reqIn(body, ec);

    ParserError rc = parser.parse(
        "multipart/form-data; boundary=---------------------------d74496d66958873e",
        body);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.boundary,
              "\r\n-----------------------------d74496d66958873e");
    ASSERT_EQ(parser.mime_fields.size(), 1);

    EXPECT_EQ(parser.mime_fields[0].fields.at("Content-Disposition"),
              "form-data; name=\"Test1\"");
    EXPECT_EQ(parser.mime_fields[0].fields.at("Other-Header"), "value=\"v1\"");
    const std::string& content0 =
        std::get<std::string>(parser.mime_fields[0].content);
    EXPECT_EQ(content0, "Data1");
}

TEST_F(MultipartTest, TestErrorHeaderWithoutColon)
{
    std::string_view body =
        "----end\r\n"
        "abc\r\n"
        "\r\n"
        "Data1\r\n"
        "----end--\r\n";
    crow::Request reqIn(body, ec);

    EXPECT_EQ(parser.parse("multipart/form-data; boundary=--end", body),
              ParserError::ERROR_UNEXPECTED_END_OF_HEADER);
}

TEST_F(MultipartTest, TestUnknownHeaderIsCorrectlyParsed)
{
    std::string_view body =
        "----end\r\n"
        "t-DiPpcccc:cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccgcccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccaaaaaa\r\n"
        "\r\n"
        "Data1\r\n"
        "----end--\r\n";

    crow::Request reqIn(body, ec);

    ParserError rc = parser.parse("multipart/form-data; boundary=--end", body);

    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.boundary, "\r\n----end");
    ASSERT_EQ(parser.mime_fields.size(), 1);

    EXPECT_EQ(
        parser.mime_fields[0].fields.at("t-DiPpcccc"),
        "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccgcccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccaaaaaa");

    const std::string& content0 =
        std::get<std::string>(parser.mime_fields[0].content);
    EXPECT_EQ(content0, "Data1");
}

TEST_F(MultipartTest, TestErrorMissingSeparatorBetweenMimeFieldsAndData)
{
    std::string_view body =
        "-----------------------------d74496d66958873e\r\n"
        "t-DiPpcccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccgcccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccaaaaaa\r\n"
        "Data1"
        "-----------------------------d74496d66958873e--";

    crow::Request reqIn(body, ec);

    ParserError rc = parser.parse(
        "multipart/form-data; boundary=---------------------------d74496d66958873e",
        body);

    EXPECT_EQ(rc, ParserError::ERROR_UNEXPECTED_END_OF_HEADER);
}

TEST_F(MultipartTest, TestDataWithoutMimeFields)
{
    std::string_view body =
        "-----------------------------d74496d66958873e\r\n"
        "\r\n"
        "Data1\r\n"
        "-----------------------------d74496d66958873e--";

    crow::Request reqIn(body, ec);

    ParserError rc = parser.parse(
        "multipart/form-data; boundary=---------------------------d74496d66958873e",
        body);

    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.boundary,
              "\r\n-----------------------------d74496d66958873e");
    ASSERT_EQ(parser.mime_fields.size(), 1);

    EXPECT_EQ(std::distance(parser.mime_fields[0].fields.begin(),
                            parser.mime_fields[0].fields.end()),
              0);
    const std::string& content0 =
        std::get<std::string>(parser.mime_fields[0].content);
    EXPECT_EQ(content0, "Data1");
}

TEST_F(MultipartTest, TestErrorMissingFinalBoundry)
{
    std::string_view body =
        "----XX\r\n"
        "Content-Disposition: form-data; name=\"Test2\"\r\n\r\n"
        "t-DiPpccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccAAAAAAAAAAAAAAABCDz\r\n"
        "\335\r\n\r\n";

    crow::Request reqIn(body, ec);

    ParserError rc = parser.parse("multipart/form-data; boundary=--XX", body);

    EXPECT_EQ(rc, ParserError::ERROR_UNEXPECTED_END_OF_INPUT);
}

TEST_F(MultipartTest, TestIgnoreDataAfterFinalBoundary)
{
    std::string_view body =
        "----XX\r\n"
        "Content-Disposition: form-data; name=\"Test1\"\r\n\r\n"
        "Data1\r\n"
        "----XX--\r\n"
        "Content-Disposition: form-data; name=\"Test2\"\r\n\r\n"
        "Data2\r\n"
        "----XX--\r\n";

    crow::Request reqIn(body, ec);

    reqIn.addHeader("Content-Type", "multipart/form-data; boundary=--XX");

    ParserError rc = parser.parse("multipart/form-data; boundary=--XX", body);

    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.boundary, "\r\n----XX");
    EXPECT_EQ(parser.mime_fields.size(), 1);

    EXPECT_EQ(parser.mime_fields[0].fields.at("Content-Disposition"),
              "form-data; name=\"Test1\"");
    const std::string& content0 =
        std::get<std::string>(parser.mime_fields[0].content);
    EXPECT_EQ(content0, "Data1");
}

TEST_F(MultipartTest, TestFinalBoundaryIsCorrectlyRecognized)
{
    std::string_view body =
        "----XX\r\n"
        "Content-Disposition: form-data; name=\"Test1\"\r\n\r\n"
        "Data1\r\n"
        "----XX-abc-\r\n"
        "StillData1\r\n"
        "----XX--\r\n";

    crow::Request reqIn(body, ec);

    ParserError rc = parser.parse("multipart/form-data; boundary=--XX", body);

    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.boundary, "\r\n----XX");
    EXPECT_EQ(parser.mime_fields.size(), 1);

    EXPECT_EQ(parser.mime_fields[0].fields.at("Content-Disposition"),
              "form-data; name=\"Test1\"");
    const std::string& content0 =
        std::get<std::string>(parser.mime_fields[0].content);
    EXPECT_EQ(content0, "Data1\r\n"
                        "----XX-abc-\r\n"
                        "StillData1");
}

} // namespace
