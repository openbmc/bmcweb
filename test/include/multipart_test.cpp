// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "duplicatable_file_handle.hpp"
#include "http_request.hpp"
#include "multipart_parser.hpp"

#include <boost/beast/http/fields.hpp>

#include <iterator>
#include <string_view>
#include <system_error>
#include <variant>
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
        "Content-Disposition: form-data; name=\"Test1\"; filename=\"test1.txt\"\r\n\r\n"
        "111111111111111111111111112222222222222222222222222222222\r\n"
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test2\"; filename=\"test2.txt\"\r\n\r\n"
        "{\r\n-----------------------------d74496d66958873e123456\r\n"
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test3\"; filename=\"test3.txt\"\r\n\r\n"
        "{\r\n--------d74496d6695887}\r\n"
        "-----------------------------d74496d66958873e--\r\n";

    std::string contentType =
        "multipart/form-data; boundary=---------------------------d74496d66958873e";

    // Using the complete parse() method as a convenience
    ParserError rc = parser.parse(contentType, body);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.boundary,
              "\r\n-----------------------------d74496d66958873e");
    EXPECT_EQ(parser.mime_fields.size(), 3);

    EXPECT_EQ(parser.mime_fields[0].fields.at("Content-Disposition"),
              "form-data; name=\"Test1\"; filename=\"test1.txt\"");

    for (const auto& field : parser.mime_fields)
    {
        EXPECT_NE(std::get_if<DuplicatableFileHandle>(&field.content), nullptr);
    }
}

TEST_F(MultipartTest, TestBadMultipartParser1)
{
    std::string_view body =
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test1\"\r\n\r\n"
        "1234567890\r\n"
        "-----------------------------d74496d66958873e\r-\r\n";

    std::string contentType =
        "multipart/form-data; boundary=---------------------------d74496d66958873e";

    ParserError rc = parser.start(contentType);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    rc = parser.parsePart(body);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    rc = parser.finish();
    EXPECT_EQ(rc, ParserError::ERROR_UNEXPECTED_END_OF_INPUT);
}

TEST_F(MultipartTest, TestBadMultipartParser2)
{
    std::string_view body =
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test1\"\r\n\r\n"
        "abcd\r\n"
        "-----------------------------d74496d66958873e-\r\n";

    std::string contentType =
        "multipart/form-data; boundary=---------------------------d74496d66958873e";

    ParserError rc = parser.start(contentType);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    rc = parser.parsePart(body);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    rc = parser.finish();
    EXPECT_EQ(rc, ParserError::ERROR_UNEXPECTED_END_OF_INPUT);
}

TEST_F(MultipartTest, TestErrorBoundaryFormat)
{
    std::string_view contentType =
        "multipart/form-data; boundary+=-----------------------------d74496d66958873e";

    EXPECT_EQ(parser.start(contentType), ParserError::ERROR_BOUNDARY_FORMAT);
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

    std::string contentType =
        "multipart/form-data; boundary=---------------------------d74496d66958873e";

    ParserError rc = parser.start(contentType);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.parsePart(body), ParserError::ERROR_BOUNDARY_CR);
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

    std::string contentType =
        "multipart/form-data; boundary=---------------------------d74496d66958873e";

    ParserError rc = parser.start(contentType);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.parsePart(body), ParserError::ERROR_BOUNDARY_LF);
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

    std::string contentType =
        "multipart/form-data; boundary=---------------------------d7449sd6d66958873e";

    ParserError rc = parser.start(contentType);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.parsePart(body), ParserError::ERROR_BOUNDARY_DATA);
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

    std::string contentType =
        "multipart/form-data; boundary=---------------------------d74496d66958873e";

    ParserError rc = parser.start(contentType);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.parsePart(body), ParserError::ERROR_EMPTY_HEADER);
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

    std::string contentType =
        "multipart/form-data; boundary=---------------------------d74496d66958873e";

    ParserError rc = parser.start(contentType);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.parsePart(body), ParserError::ERROR_HEADER_NAME);
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

    std::string contentType =
        "multipart/form-data; boundary=---------------------------d74496d66958873e";

    ParserError rc = parser.start(contentType);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.parsePart(body), ParserError::ERROR_HEADER_VALUE);
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

    std::string contentType =
        "multipart/form-data; boundary=---------------------------d74496d66958873e";

    ParserError rc = parser.start(contentType);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.parsePart(body), ParserError::ERROR_HEADER_ENDING);
}

TEST_F(MultipartTest, TestGoodMultipartParserMultipleHeaders)
{
    std::string_view body =
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test1\"; filename=\"test1.txt\"\r\n"
        "Other-Header: value=\"v1\"\r\n"
        "\r\n"
        "Data1\r\n"
        "-----------------------------d74496d66958873e--";

    std::string contentType =
        "multipart/form-data; boundary=---------------------------d74496d66958873e";

    ParserError rc = parser.start(contentType);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    rc = parser.parsePart(body);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    rc = parser.finish();
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.boundary,
              "\r\n-----------------------------d74496d66958873e");
    ASSERT_EQ(parser.mime_fields.size(), 1);

    EXPECT_EQ(parser.mime_fields[0].fields.at("Content-Disposition"),
              "form-data; name=\"Test1\"; filename=\"test1.txt\"");
    EXPECT_EQ(parser.mime_fields[0].fields.at("Other-Header"), "value=\"v1\"");

    EXPECT_NE(
        std::get_if<DuplicatableFileHandle>(&parser.mime_fields[0].content),
        nullptr);
}

TEST_F(MultipartTest, TestErrorHeaderWithoutColon)
{
    std::string_view body =
        "----end\r\n"
        "abc\r\n"
        "\r\n"
        "Data1\r\n"
        "----end--\r\n";

    std::string contentType = "multipart/form-data; boundary=--end";

    ParserError rc = parser.start(contentType);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.parsePart(body),
              ParserError::ERROR_UNEXPECTED_END_OF_HEADER);
}

TEST_F(MultipartTest, TestUnknownHeaderIsCorrectlyParsed)
{
    std::string_view body =
        "----end\r\n"
        "Content-Disposition: form-data; name=\"Test1\"; filename=\"test.txt\"\r\n"
        "t-DiPpcccc:cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccgcccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccaaaaaa\r\n"
        "\r\n"
        "Data1\r\n"
        "----end--\r\n";

    std::string contentType = "multipart/form-data; boundary=--end";

    ParserError rc = parser.start(contentType);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    rc = parser.parsePart(body);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    rc = parser.finish();
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.boundary, "\r\n----end");
    ASSERT_EQ(parser.mime_fields.size(), 1);

    EXPECT_EQ(parser.mime_fields[0].fields.at("Content-Disposition"),
              "form-data; name=\"Test1\"; filename=\"test.txt\"");

    EXPECT_EQ(
        parser.mime_fields[0].fields.at("t-DiPpcccc"),
        "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccgcccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccaaaaaa");

    EXPECT_NE(
        std::get_if<DuplicatableFileHandle>(&parser.mime_fields[0].content),
        nullptr);
}

TEST_F(MultipartTest, TestErrorMissingSeparatorBetweenMimeFieldsAndData)
{
    std::string_view body =
        "-----------------------------d74496d66958873e\r\n"
        "t-DiPpcccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccgcccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccaaaaaa\r\n"
        "Data1"
        "-----------------------------d74496d66958873e--";

    std::string contentType =
        "multipart/form-data; boundary=---------------------------d74496d66958873e";

    ParserError rc = parser.start(contentType);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.parsePart(body),
              ParserError::ERROR_UNEXPECTED_END_OF_HEADER);
}

TEST_F(MultipartTest, TestDataWithoutMimeFields)
{
    std::string_view body =
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"data.txt\"\r\n"
        "\r\n"
        "Data1\r\n"
        "-----------------------------d74496d66958873e--";

    std::string contentType =
        "multipart/form-data; boundary=---------------------------d74496d66958873e";

    ParserError rc = parser.start(contentType);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    rc = parser.parsePart(body);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    rc = parser.finish();
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.boundary,
              "\r\n-----------------------------d74496d66958873e");
    ASSERT_EQ(parser.mime_fields.size(), 1);

    EXPECT_EQ(parser.mime_fields[0].fields.at("Content-Disposition"),
              "form-data; name=\"file\"; filename=\"data.txt\"");

    EXPECT_NE(
        std::get_if<DuplicatableFileHandle>(&parser.mime_fields[0].content),
        nullptr);
}

TEST_F(MultipartTest, TestErrorMissingFinalBoundry)
{
    std::string_view body =
        "----XX\r\n"
        "Content-Disposition: form-data; name=\"Test2\"\r\n\r\n"
        "t-DiPpccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccAAAAAAAAAAAAAAABCDz\r\n"
        "\335\r\n\r\n";

    std::string contentType = "multipart/form-data; boundary=--XX";

    ParserError rc = parser.start(contentType);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    rc = parser.parsePart(body);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.finish(), ParserError::ERROR_UNEXPECTED_END_OF_INPUT);
}

TEST_F(MultipartTest, TestIgnoreDataAfterFinalBoundary)
{
    std::string_view body =
        "----XX\r\n"
        "Content-Disposition: form-data; name=\"Test1\"; filename=\"test1.txt\"\r\n\r\n"
        "Data1\r\n"
        "----XX--\r\n"
        "Content-Disposition: form-data; name=\"Test2\"\r\n\r\n"
        "Data2\r\n"
        "----XX--\r\n";

    std::string contentType = "multipart/form-data; boundary=--XX";

    ParserError rc = parser.start(contentType);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    rc = parser.parsePart(body);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    rc = parser.finish();
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.boundary, "\r\n----XX");
    EXPECT_EQ(parser.mime_fields.size(), 1);

    EXPECT_EQ(parser.mime_fields[0].fields.at("Content-Disposition"),
              "form-data; name=\"Test1\"; filename=\"test1.txt\"");

    EXPECT_NE(
        std::get_if<DuplicatableFileHandle>(&parser.mime_fields[0].content),
        nullptr);
}

TEST_F(MultipartTest, TestFinalBoundaryIsCorrectlyRecognized)
{
    std::string_view body =
        "----XX\r\n"
        "Content-Disposition: form-data; name=\"Test1\"; filename=\"test1.txt\"\r\n\r\n"
        "Data1\r\n"
        "----XX-abc-\r\n"
        "StillData1\r\n"
        "----XX--\r\n";

    std::string contentType = "multipart/form-data; boundary=--XX";

    ParserError rc = parser.start(contentType);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    rc = parser.parsePart(body);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    rc = parser.finish();
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.boundary, "\r\n----XX");
    EXPECT_EQ(parser.mime_fields.size(), 1);

    EXPECT_EQ(parser.mime_fields[0].fields.at("Content-Disposition"),
              "form-data; name=\"Test1\"; filename=\"test1.txt\"");

    EXPECT_NE(
        std::get_if<DuplicatableFileHandle>(&parser.mime_fields[0].content),
        nullptr);
}

} // namespace
