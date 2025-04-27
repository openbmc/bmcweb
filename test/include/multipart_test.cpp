// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "multipart_parser.hpp"

#include <iterator>
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
};

constexpr std::string_view goodMultipartParserBody =
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

TEST_F(MultipartTest, TestGoodMultipartParser)
{
    ParserError rc =
        parser.parse("multipart/form-data; "
                     "boundary=---------------------------d74496d66958873e",
                     goodMultipartParserBody);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.boundary,
              "\r\n-----------------------------d74496d66958873e");
    EXPECT_EQ(parser.mime_fields.size(), 3);

    EXPECT_EQ(parser.mime_fields[0].fields["Content-Disposition"],
              "form-data; name=\"Test1\"");
    EXPECT_EQ(parser.mime_fields[0].content,
              "111111111111111111111111112222222222222222222222222222222");

    EXPECT_EQ(parser.mime_fields[1].fields["Content-Disposition"],
              "form-data; name=\"Test2\"");
    EXPECT_EQ(parser.mime_fields[1].content,
              "{\r\n-----------------------------d74496d66958873e123456");
    EXPECT_EQ(parser.mime_fields[2].fields["Content-Disposition"],
              "form-data; name=\"Test3\"");
    EXPECT_EQ(parser.mime_fields[2].content, "{\r\n--------d74496d6695887}");
}

TEST(MultipartTestChunked, TestGoodMultipartParserChunked)
{
    for (size_t chunkSize : {1UZ, 2UZ, 4UZ, goodMultipartParserBody.size()})
    {
        MultipartParser parser;

        EXPECT_EQ(
            parser.start(
                "multipart/form-data; boundary=---------------------------d74496d66958873e"),
            ParserError::PARSER_SUCCESS);

        std::string_view remaining = goodMultipartParserBody;
        while (!remaining.empty())
        {
            std::string_view chunk = remaining.substr(0, chunkSize);
            remaining.remove_prefix(chunk.size());
            ASSERT_EQ(parser.parsePart(chunk), ParserError::PARSER_SUCCESS);
        }

        EXPECT_EQ(parser.finish(), ParserError::PARSER_SUCCESS);

        EXPECT_EQ(parser.boundary,
                  "\r\n-----------------------------d74496d66958873e");
        EXPECT_EQ(parser.mime_fields.size(), 3);

        EXPECT_EQ(parser.mime_fields[0].fields["Content-Disposition"],
                  "form-data; name=\"Test1\"");

        EXPECT_EQ(parser.mime_fields[0].content,
                  "111111111111111111111111112222222222222222222222222222222");

        EXPECT_EQ(parser.mime_fields[1].fields["Content-Disposition"],
                  "form-data; name=\"Test2\"");

        EXPECT_EQ(parser.mime_fields[1].content,
                  "{\r\n-----------------------------d74496d66958873e123456");
        EXPECT_EQ(parser.mime_fields[2].fields["Content-Disposition"],
                  "form-data; name=\"Test3\"");
        EXPECT_EQ(parser.mime_fields[2].content,
                  "{\r\n--------d74496d6695887}");
    }
}

TEST_F(MultipartTest, TestBadMultipartParser1)
{
    std::string_view body =
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test1\"\r\n\r\n"
        "1234567890\r\n"
        "-----------------------------d74496d66958873e\r-\r\n";

    ParserError rc =
        parser.parse("multipart/form-data; "
                     "boundary=---------------------------d74496d66958873e",
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

    ParserError rc =
        parser.parse("multipart/form-data; "
                     "boundary=---------------------------d74496d66958873e",
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

    EXPECT_EQ(
        parser.parse("multipart/form-data; "
                     "boundary+=-----------------------------d74496d66958873e",
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

    EXPECT_EQ(
        parser.parse("multipart/form-data; "
                     "boundary=---------------------------d74496d66958873e",
                     body),
        ParserError::ERROR_BOUNDARY_FORMAT);
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

    EXPECT_EQ(
        parser.parse("multipart/form-data; "
                     "boundary=---------------------------d74496d66958873e",
                     body),
        ParserError::ERROR_BOUNDARY_FORMAT);
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

    EXPECT_EQ(
        parser.parse("multipart/form-data; "
                     "boundary=---------------------------d7449sd6d66958873e",
                     body),
        ParserError::ERROR_BOUNDARY_FORMAT);
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

    EXPECT_EQ(
        parser.parse("multipart/form-data; "
                     "boundary=---------------------------d74496d66958873e",
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

    EXPECT_EQ(
        parser.parse("multipart/form-data; "
                     "boundary=---------------------------d74496d66958873e",
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

    EXPECT_EQ(
        parser.parse("multipart/form-data; "
                     "boundary=---------------------------d74496d66958873e",
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

    EXPECT_EQ(
        parser.parse("multipart/form-data; "
                     "boundary=---------------------------d74496d66958873e",
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

    ParserError rc =
        parser.parse("multipart/form-data; "
                     "boundary=---------------------------d74496d66958873e",
                     body);
    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.boundary,
              "\r\n-----------------------------d74496d66958873e");
    ASSERT_EQ(parser.mime_fields.size(), 1);

    EXPECT_EQ(parser.mime_fields[0].fields["Content-Disposition"],
              "form-data; name=\"Test1\"");
    EXPECT_EQ(parser.mime_fields[0].fields["Other-Header"], "value=\"v1\"");
    EXPECT_EQ(parser.mime_fields[0].content, "Data1");
}

TEST_F(MultipartTest, TestErrorHeaderWithoutColon)
{
    std::string_view body =
        "----end\r\n"
        "abc\r\n"
        "\r\n"
        "Data1\r\n"
        "----end--\r\n";

    ParserError rc = parser.parse("multipart/form-data; "
                                  "boundary=--end",
                                  body);

    EXPECT_EQ(rc, ParserError::ERROR_UNEXPECTED_END_OF_HEADER);
}

TEST_F(MultipartTest, TestUnknownHeaderIsCorrectlyParsed)
{
    std::string_view body =
        "----end\r\n"
        "t-DiPpcccc:cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccgcccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccaaaaaa\r\n"
        "\r\n"
        "Data1\r\n"
        "----end--\r\n";

    ParserError rc = parser.parse("multipart/form-data; "
                                  "boundary=--end",
                                  body);

    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.boundary, "\r\n----end");
    ASSERT_EQ(parser.mime_fields.size(), 1);

    EXPECT_EQ(
        parser.mime_fields[0].fields["t-DiPpcccc"],
        "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccgcccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccaaaaaa");
    EXPECT_EQ(parser.mime_fields[0].content, "Data1");
}

TEST_F(MultipartTest, TestErrorMissingSeparatorBetweenMimeFieldsAndData)
{
    std::string_view body =
        "-----------------------------d74496d66958873e\r\n"
        "t-DiPpcccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccgcccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccaaaaaa\r\n"
        "Data1"
        "-----------------------------d74496d66958873e--";

    ParserError rc =
        parser.parse("multipart/form-data; "
                     "boundary=---------------------------d74496d66958873e",
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

    ParserError rc =
        parser.parse("multipart/form-data; "
                     "boundary=---------------------------d74496d66958873e",
                     body);

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
    std::string_view body =
        "----XX\r\n"
        "Content-Disposition: form-data; name=\"Test2\"\r\n\r\n"
        "t-DiPpccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccAAAAAAAAAAAAAAABCDz\r\n"
        "\335\r\n\r\n";

    ParserError rc = parser.parse("multipart/form-data; boundary=--XX", body);

    EXPECT_EQ(rc, ParserError::ERROR_UNEXPECTED_END_OF_INPUT);
}

TEST_F(MultipartTest, TestErrorOnDataAfterFinalBoundary)
{
    std::string_view body =
        "----XX\r\n"
        "Content-Disposition: form-data; name=\"Test1\"\r\n\r\n"
        "Data1\r\n"
        "----XX--\r\n"
        "Content-Disposition: form-data; name=\"Test2\"\r\n\r\n"
        "Data2\r\n"
        "----XX--\r\n";

    ParserError rc = parser.parse("multipart/form-data; boundary=--XX", body);

    ASSERT_EQ(rc, ParserError::ERROR_DATA_AFTER_FINAL_BOUNDARY);
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

    ParserError rc = parser.parse("multipart/form-data; boundary=--XX", body);

    ASSERT_EQ(rc, ParserError::PARSER_SUCCESS);

    EXPECT_EQ(parser.boundary, "\r\n----XX");
    EXPECT_EQ(parser.mime_fields.size(), 1);

    EXPECT_EQ(parser.mime_fields[0].fields["Content-Disposition"],
              "form-data; name=\"Test1\"");
    EXPECT_EQ(parser.mime_fields[0].content,
              "Data1\r\n"
              "----XX-abc-\r\n"
              "StillData1");
}

} // namespace
