#include "http_request.hpp"
#include "http_response.hpp"
#include "include/ibm/management_console_rest.hpp"
#include "nlohmann/json.hpp"

#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

#include <string>

#include <gtest/gtest.h>

namespace crow
{
namespace ibm_mc
{
TEST(IsValidConfigFileName, FileNameValidCharReturnsTrue)
{
    nlohmann::json jsonValue;
    EXPECT_TRUE(isValidConfigFileName("GoodConfigFile", jsonValue));
}
TEST(IsValidConfigFileName, FileNameInvalidCharReturnsFalse)
{
    nlohmann::json jsonValue;
    EXPECT_FALSE(isValidConfigFileName("Bad@file", jsonValue));
}
TEST(IsValidConfigFileName, FileNameInvalidPathReturnsFalse)
{
    nlohmann::json jsonValue;
    EXPECT_FALSE(
        isValidConfigFileName("/../../../../../etc/badpath", jsonValue));
    EXPECT_FALSE(isValidConfigFileName("/../../etc/badpath", jsonValue));

    EXPECT_FALSE(isValidConfigFileName("/mydir/configFile", jsonValue));
}
TEST(IsValidConfigFileName, EmptyFileNameReturnsFalse)
{
    nlohmann::json jsonValue;
    EXPECT_FALSE(isValidConfigFileName("", jsonValue));
}
TEST(IsValidConfigFileName, SlashFileNameReturnsFalse)
{
    nlohmann::json jsonValue;
    EXPECT_FALSE(isValidConfigFileName("/", jsonValue));
}
TEST(IsValidConfigFileName, FileNameMoreThan20CharReturnsFalse)
{
    nlohmann::json jsonValue;
    EXPECT_FALSE(isValidConfigFileName("BadfileBadfileBadfile", jsonValue));
}

TEST(handleFileUpload, contentNotAcceptable1)
{
    std::error_code ec;
    crow::Response res;
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>(std::move(res));
    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.set("Content-Type",
            "multipart/form-data; "
            "boundary=---------------------------d74496d66958873e");

    req.body() =
        "-----------------------------d74495d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test1\"\r\n\r\n"
        "1111111111111111111111111122222222222222222222222222222223333333333333333333333333333333333444444444444444444444444444\r\n"
        "1111111111111111111111111122222222222222222222222222222223333333333333333333333333333333333444444444444444444444444444\r\n"
        "1111111111111111111111111122222222222222222222222222222223333333333333333333333333333333333444444444444444444444444444\r\n"
        "1111111111111111111111111122222222222222222222222222222223333333333333333333333333333333333444444444444444444444444444\r\n"
        "1111111111111111111111111122222222222222222222222222222223333333333333333333333333333333333444444444444444444444444444\r\n"
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test2\"\r\n\r\n"
        "{\r\n-----------------------------d74496d66958873e123456\r\n"
        "1111111111111111111111111122222222222222222222222222222223333333333333333333333333333333333444444444444444444444444444\r\n"
        "1111111111111111111111111122222222222222222222222222222223333333333333333333333333333333333444444444444444444444444444\r\n"
        "1111111111111111111111111122222222222222222222222222222223333333333333333333333333333333333444444444444444444444444444\r\n"
        "1111111111111111111111111122222222222222222222222222222223333333333333333333333333333333333444444444444444444444444444\r\n"
        "1111111111111111111111111122222222222222222222222222222223333333333333333333333333333333333444444444444444444444444444\r\n"
        "1111111111111111111111111122222222222222222222222222222223333333333333333333333333333333333444444444444444444444444444\r\n"
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test3\"\r\n\r\n"
        "{\r\n--------d74496d6695887}\r\n"
        "1111111111111111111111111122222222222222222222222222222223333333333333333333333333333333333444444444444444444444444444\r\n"
        "1111111111111111111111111122222222222222222222222222222223333333333333333333333333333333333444444444444444444444444444\r\n"
        "1111111111111111111111111122222222222222222222222222222223333333333333333333333333333333333444444444444444444444444444\r\n"
        "1111111111111111111111111122222222222222222222222222222223333333333333333333333333333333333444444444444444444444444444\r\n"
        "1111111111111111111111111122222222222222222222222222222223333333333333333333333333333333333444444444444444444444444444\r\n"
        "1111111111111111111111111122222222222222222222222222222223333333333333333333333333333333333444444444444444444444444444\r\n"
        "-----------------------------d74496d66958873e--\r\n";
    req.method(boost::beast::http::verb::put);
    crow::Request reqIn(req, ec);
    const std::string& fileID = "Test1";
    handleFileUpload(reqIn, asyncResp, fileID);
    EXPECT_EQ(asyncResp->res.jsonValue["Description"],
              std::string(contentNotAcceptableMsg));
}

TEST(handleFileUpload, contentNotAcceptable2)
{
    std::error_code ec;
    crow::Response res;
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>(std::move(res));
    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.set("Content-Type",
            "application/octet-stream; "
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
    req.method(boost::beast::http::verb::post);
    crow::Request reqIn(req, ec);
    const std::string& fileID = "Test1";

    handleFileUpload(reqIn, asyncResp, fileID);
    EXPECT_EQ(asyncResp->res.jsonValue["Description"],
              std::string(contentNotAcceptableMsg));
}
TEST(handleFileUpload, contentAcceptable1)
{
    std::error_code ec;
    boost::beast::http::request<boost::beast::http::string_body> req{};
    crow::Response res;
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>(std::move(res));
    req.set("Content-Type",
            "multipart/form-data; "
            "boundary=---------------------------d74496d66958873e");

    req.body() = "-----------------------------d74496d66958873e\r\n"
                 "Content-Disposition: form-data; name=\"\\tmp\\Test1\"\r\n\r\n"
                 "111111111111111111111111112222222222222222222222222222222\r\n"
                 "-----------------------------d74496d66958873e\r\n"
                 "Content-Disposition: form-data; name=\"\\tmp\\Test2\"\r\n\r\n"
                 "{\r\n-----------------------------d74496d66958873e123456\r\n"
                 "-----------------------------d74496d66958873e\r\n"
                 "Content-Disposition: form-data; name=\"\\tmp\\Test3\"\r\n\r\n"
                 "{\r\n--------d74496d6695887}\r\n"
                 "-----------------------------d74496d66958873e--\r\n";
    req.method(boost::beast::http::verb::post);
    crow::Request reqIn(req, ec);
    const std::string& fileID = "Test1";

    handleFileUpload(reqIn, asyncResp, fileID);
    EXPECT_NE(asyncResp->res.jsonValue["Description"],
              std::string(contentNotAcceptableMsg));
    EXPECT_NE(asyncResp->res.jsonValue["Description"],
              std::string(resourceNotFoundMsg));
    EXPECT_NE(asyncResp->res.jsonValue["Description"],
              std::string(badRequestMsg));
}
TEST(handleFileUpload, contentAcceptable2)
{
    std::error_code ec;
    crow::Response res;
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>(std::move(res));
    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.set("Content-Type", "application/octet-stream");

    req.body() = "-----------------------------d74496d66958873e";
    req.method(boost::beast::http::verb::put);
    crow::Request reqIn(req, ec);
    const std::string& fileID = "Test1";

    handleFileUpload(reqIn, asyncResp, fileID);
    EXPECT_NE(asyncResp->res.jsonValue["Description"],
              std::string(contentNotAcceptableMsg));
    EXPECT_NE(asyncResp->res.jsonValue["Description"],
              std::string(resourceNotFoundMsg));
    EXPECT_NE(asyncResp->res.jsonValue["Description"],
              std::string(badRequestMsg));
}

} // namespace ibm_mc
} // namespace crow
