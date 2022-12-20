#include "http_request.hpp"
#include "http_response.hpp"
#include "include/ibm/management_console_rest.hpp"
#include "nlohmann/json.hpp"

#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

#include <string>

#include <gtest/gtest.h> // IWYU pragma: keep

// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
// IWYU pragma: no_include "gtest/gtest_pred_impl.h"

namespace crow
{
namespace ibm_mc
{

TEST(IsValidConfigFileName, FileNameValidCharReturnsTrue)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    EXPECT_TRUE(
        isValidConfigFileName("GoodConfigFile", asyncResp->res.jsonValue));
}

TEST(IsValidConfigFileName, FileNameInvalidCharReturnsFalse)
{

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    EXPECT_FALSE(isValidConfigFileName("Bad@file", asyncResp->res.jsonValue));
}

TEST(IsValidConfigFileName, FileNameInvalidPathReturnsFalse)
{

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    EXPECT_FALSE(isValidConfigFileName("/../../../../../etc/badpath",
                                       asyncResp->res.jsonValue));
    EXPECT_FALSE(
        isValidConfigFileName("/../../etc/badpath", asyncResp->res.jsonValue));
    EXPECT_FALSE(
        isValidConfigFileName("/mydir/configFile", asyncResp->res.jsonValue));
}

TEST(IsValidConfigFileName, EmptyFileNameReturnsFalse)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    EXPECT_FALSE(isValidConfigFileName("", asyncResp->res.jsonValue));
}

TEST(IsValidConfigFileName, SlashFileNameReturnsFalse)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    EXPECT_FALSE(isValidConfigFileName("/", asyncResp->res.jsonValue));
}

TEST(IsValidConfigFileName, FileNameMoreThan20CharReturnsFalse)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    EXPECT_FALSE(isValidConfigFileName("BadfileBadfileBadfile",
                                       asyncResp->res.jsonValue));
}

TEST(handleFileUpload, TestMultipartWithPut)
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
        "111111111111111111111111112222222222222222222222222222222333333"
        "3333333333333333333333333333444444444444444444444444444\r\n"
        "111111111111111111111111112222222222222222222222222222222333333"
        "3333333333333333333333333333444444444444444444444444444\r\n"
        "111111111111111111111111112222222222222222222222222222222333333"
        "3333333333333333333333333333444444444444444444444444444\r\n"
        "111111111111111111111111112222222222222222222222222222222333333"
        "3333333333333333333333333333444444444444444444444444444\r\n"
        "111111111111111111111111112222222222222222222222222222222333333"
        "3333333333333333333333333333444444444444444444444444444\r\n"
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test2\"\r\n\r\n"
        "{\r\n-----------------------------d74496d66958873e123456\r\n"
        "111111111111111111111111112222222222222222222222222222222333333"
        "3333333333333333333333333333444444444444444444444444444\r\n"
        "111111111111111111111111112222222222222222222222222222222333333"
        "3333333333333333333333333333444444444444444444444444444\r\n"
        "111111111111111111111111112222222222222222222222222222222333333"
        "3333333333333333333333333333444444444444444444444444444\r\n"
        "111111111111111111111111112222222222222222222222222222222333333"
        "3333333333333333333333333333444444444444444444444444444\r\n"
        "111111111111111111111111112222222222222222222222222222222333333"
        "3333333333333333333333333333444444444444444444444444444\r\n"
        "111111111111111111111111112222222222222222222222222222222333333"
        "3333333333333333333333333333444444444444444444444444444\r\n"
        "-----------------------------d74496d66958873e\r\n"
        "Content-Disposition: form-data; name=\"Test3\"\r\n\r\n"
        "{\r\n--------d74496d6695887}\r\n"
        "111111111111111111111111112222222222222222222222222222222333333"
        "3333333333333333333333333333444444444444444444444444444\r\n"
        "111111111111111111111111112222222222222222222222222222222333333"
        "3333333333333333333333333333444444444444444444444444444\r\n"
        "111111111111111111111111112222222222222222222222222222222333333"
        "3333333333333333333333333333444444444444444444444444444\r\n"
        "111111111111111111111111112222222222222222222222222222222333333"
        "3333333333333333333333333333444444444444444444444444444\r\n"
        "111111111111111111111111112222222222222222222222222222222333333"
        "3333333333333333333333333333444444444444444444444444444\r\n"
        "111111111111111111111111112222222222222222222222222222222333333"
        "3333333333333333333333333333444444444444444444444444444\r\n"
        "-----------------------------d74496d66958873e--\r\n";
    req.method(boost::beast::http::verb::put);
    crow::Request reqIn(req, ec);
    const std::string& fileID = "Test1";
    handleFileUpload(reqIn, asyncResp, fileID);
    EXPECT_EQ(asyncResp->res.jsonValue["Description"],
              std::string(contentNotAcceptableMsg));
}

TEST(handleFileUpload, TestOctetStreamWithPost)
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

} // namespace ibm_mc
} // namespace crow
