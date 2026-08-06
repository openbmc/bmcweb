// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "update_service.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/errc.hpp>
#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

using nlohmann::literals::operator""_json;

TEST(UpdateService, ParseHTTSPPostitive)
{
    crow::Response res;
    {
        // No protocol, schema on url
        std::optional<boost::urls::url> ret =
            parseSimpleUpdateUrl("https://1.1.1.1/path", std::nullopt, res);
        ASSERT_TRUE(ret);
        if (!ret)
        {
            return;
        }
        EXPECT_EQ(ret->encoded_host_and_port(), "1.1.1.1");
        EXPECT_EQ(ret->encoded_path(), "/path");
        EXPECT_EQ(ret->scheme(), "https");
    }
    {
        // Protocol, no schema on url
        std::optional<boost::urls::url> ret =
            parseSimpleUpdateUrl("1.1.1.1/path", "HTTPS", res);
        ASSERT_TRUE(ret);
        if (!ret)
        {
            return;
        }
        EXPECT_EQ(ret->encoded_host_and_port(), "1.1.1.1");
        EXPECT_EQ(ret->encoded_path(), "/path");
        EXPECT_EQ(ret->scheme(), "https");
    }
    {
        // Both protocol and schema on url
        std::optional<boost::urls::url> ret =
            parseSimpleUpdateUrl("https://1.1.1.1/path", "HTTPS", res);
        ASSERT_TRUE(ret);
        if (!ret)
        {
            return;
        }
        EXPECT_EQ(ret->encoded_host_and_port(), "1.1.1.1");
        EXPECT_EQ(ret->encoded_path(), "/path");
        EXPECT_EQ(ret->scheme(), "https");
    }
}

TEST(UpdateService, ParseHTTPSPostitive)
{
    crow::Response res;
    {
        // No protocol, schema on url
        std::optional<boost::urls::url> ret =
            parseSimpleUpdateUrl("https://1.1.1.1/path", std::nullopt, res);
        ASSERT_TRUE(ret);
        if (!ret)
        {
            return;
        }
        EXPECT_EQ(ret->encoded_host_and_port(), "1.1.1.1");
        EXPECT_EQ(ret->encoded_path(), "/path");
        EXPECT_EQ(ret->scheme(), "https");
    }
    {
        // Protocol, no schema on url
        std::optional<boost::urls::url> ret =
            parseSimpleUpdateUrl("1.1.1.1/path", "HTTPS", res);
        ASSERT_TRUE(ret);
        if (!ret)
        {
            return;
        }
        EXPECT_EQ(ret->encoded_host_and_port(), "1.1.1.1");
        EXPECT_EQ(ret->encoded_path(), "/path");
        EXPECT_EQ(ret->scheme(), "https");
    }
    {
        // Both protocol and schema on url with path
        std::optional<boost::urls::url> ret =
            parseSimpleUpdateUrl("https://1.1.1.1/path", "HTTPS", res);
        ASSERT_TRUE(ret);
        if (!ret)
        {
            return;
        }
        EXPECT_EQ(ret->encoded_host_and_port(), "1.1.1.1");
        EXPECT_EQ(ret->encoded_path(), "/path");
        EXPECT_EQ(ret->scheme(), "https");
    }
    {
        // Both protocol and schema on url without path
        std::optional<boost::urls::url> ret =
            parseSimpleUpdateUrl("https://1.1.1.1", "HTTPS", res);
        ASSERT_TRUE(ret);
        if (!ret)
        {
            return;
        }
        EXPECT_EQ(ret->encoded_host_and_port(), "1.1.1.1");
        EXPECT_EQ(ret->encoded_path(), "/");
        EXPECT_EQ(ret->scheme(), "https");
    }
    {
        // Both protocol and schema on url without path
        std::optional<boost::urls::url> ret =
            parseSimpleUpdateUrl("https://[2001:db8::1]", "HTTPS", res);
        ASSERT_TRUE(ret);
        if (!ret)
        {
            return;
        }
        EXPECT_EQ(ret->encoded_host_and_port(), "[2001:db8::1]");
        EXPECT_EQ(ret->encoded_path(), "/");
        EXPECT_EQ(ret->scheme(), "https");
    }
}

TEST(UpdateService, ParseHTTPSNegative)
{
    crow::Response res;
    // No protocol, no schema
    ASSERT_EQ(parseSimpleUpdateUrl("1.1.1.1/path", std::nullopt, res),
              std::nullopt);
    // No host
    ASSERT_EQ(parseSimpleUpdateUrl("/path", "HTTPS", res), std::nullopt);
}

TEST(UpdateService, MissingContentTypeReturnsHeaderMissing)
{
    std::shared_ptr<bmcweb::AsyncResp> asyncResp =
        std::make_shared<bmcweb::AsyncResp>();
    crow::Request request;
    ASSERT_TRUE(request.target("/redfish/v1/UpdateService/update"));
    request.method(boost::beast::http::verb::post);

    crow::App app;
    handleUpdateServicePost(app, request, asyncResp);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::bad_request);
    EXPECT_EQ(asyncResp->res.jsonValue,
              R"({
                  "error": {
                      "code": "Base.1.19.HeaderMissing",
                      "message": "Required header 'Content-Type' is missing in the request.",
                      "@Message.ExtendedInfo": [
                          {
                              "@odata.type": "#Message.v1_1_1.Message",
                              "Message": "Required header 'Content-Type' is missing in the request.",
                              "MessageArgs": ["Content-Type"],
                              "MessageId": "Base.1.19.HeaderMissing",
                              "MessageSeverity": "Critical",
                              "Resolution": "Resubmit the request with the required request header."
                          }
                      ]
                  }
              })"_json);
}

TEST(UpdateService, EmptyContentTypeReturnsHeaderMissing)
{
    std::shared_ptr<bmcweb::AsyncResp> asyncResp =
        std::make_shared<bmcweb::AsyncResp>();
    crow::Request request;
    ASSERT_TRUE(request.target("/redfish/v1/UpdateService/update"));
    request.method(boost::beast::http::verb::post);
    request.addHeader(boost::beast::http::field::content_type, "");

    crow::App app;
    handleUpdateServicePost(app, request, asyncResp);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::bad_request);
    EXPECT_EQ(asyncResp->res.jsonValue,
              R"({
                  "error": {
                      "code": "Base.1.19.HeaderMissing",
                      "message": "Required header 'Content-Type' is missing in the request.",
                      "@Message.ExtendedInfo": [
                          {
                              "@odata.type": "#Message.v1_1_1.Message",
                              "Message": "Required header 'Content-Type' is missing in the request.",
                              "MessageArgs": ["Content-Type"],
                              "MessageId": "Base.1.19.HeaderMissing",
                              "MessageSeverity": "Critical",
                              "Resolution": "Resubmit the request with the required request header."
                          }
                      ]
                  }
              })"_json);
}

TEST(UpdateService, InvalidContentTypeReturnsCompleteHeader)
{
    std::shared_ptr<bmcweb::AsyncResp> asyncResp =
        std::make_shared<bmcweb::AsyncResp>();
    crow::Request request;
    ASSERT_TRUE(request.target("/redfish/v1/UpdateService/update"));
    request.method(boost::beast::http::verb::post);
    request.addHeader(boost::beast::http::field::content_type,
                      "application/json");

    crow::App app;
    handleUpdateServicePost(app, request, asyncResp);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::bad_request);
    EXPECT_EQ(asyncResp->res.jsonValue,
              R"({
                  "error": {
                      "code": "Base.1.19.HeaderInvalid",
                      "message": "Header 'Content-Type: application/json' is invalid.",
                      "@Message.ExtendedInfo": [
                          {
                              "@odata.type": "#Message.v1_1_1.Message",
                              "Message": "Header 'Content-Type: application/json' is invalid.",
                              "MessageArgs": ["Content-Type: application/json"],
                              "MessageId": "Base.1.19.HeaderInvalid",
                              "MessageSeverity": "Critical",
                              "Resolution": "Resubmit the request with a valid request header."
                          }
                      ]
                  }
              })"_json);
}

// A Software.Version object that omits the deprecated Purpose property must
// still succeed: the handler must not fail the resource and must skip the
// Purpose-derived Description.
TEST(UpdateService, MissingPurposeIsToleratedAndSkipsDescription)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::DBusPropertiesMap props = {
        {"Version", std::string("anyVersion")}};
    getSoftwareVersionCallback(asyncResp, "swId", {}, props);

    EXPECT_NE(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("Description"));
}

// When Purpose is present, Description is the trailing token of the
// VersionPurpose enum string plus " image"; exercise that transform.
TEST(UpdateService, PurposeIsTranslatedToDescription)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::DBusPropertiesMap props = {
        {"Version", std::string("anyVersion")},
        {"Purpose", std::string("xyz.openbmc_project.Software.Version."
                                "VersionPurpose.BMC")}};
    getSoftwareVersionCallback(asyncResp, "swId", {}, props);

    EXPECT_NE(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_EQ(asyncResp->res.jsonValue["Description"], "BMC image");
}

// Version is mandatory; its absence is an internal error.
TEST(UpdateService, MissingVersionIsError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    getSoftwareVersionCallback(asyncResp, "swId", {},
                               dbus::utility::DBusPropertiesMap{});

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(UpdateService, StartUpdateErrorReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    crow::Request req;
    handleStartUpdate(
        asyncResp, task::Payload{req}, "/software",
        boost::system::errc::make_error_code(boost::system::errc::io_error),
        sdbusplus::object_path{});

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}
} // namespace
} // namespace redfish
