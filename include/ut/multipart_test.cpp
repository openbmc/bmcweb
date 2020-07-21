#include <http_utility.hpp>
#include <multipart_parser.hpp>

#include <map>

#include "gmock/gmock.h"

TEST(MultipartTest, MultipartParser)
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

    std::map<std::string, std::string> testMap = {
        {"form-data; name=\"Test1\"", "{\"Key1\": 112233}"},
        {"form-data; name=\"Test2\"", "123456"}};

    EXPECT_EQ(parser.mime_fields.size(), 2);
    for (const FormPart& formpart : parser.mime_fields)
    {
        boost::beast::http::fields::const_iterator it =
            formpart.fields.find("Content-Disposition");
        ASSERT_NE(it, formpart.fields.end());
        ASSERT_NE(testMap.find(std::string(it->value())), testMap.end());
        ASSERT_EQ(testMap.at(std::string(it->value())), formpart.content);
    }
}