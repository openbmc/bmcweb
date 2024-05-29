#include "json_html_serializer.hpp"

#include <nlohmann/json.hpp>

#include <string>

#include <gtest/gtest.h> // IWYU pragma: keep

// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
// IWYU pragma: no_include "gtest/gtest_pred_impl.h"

namespace json_html_util
{
namespace
{

const std::string boilerplateStart =
    "<html>\n"
    "<head>\n"
    "<title>Redfish API</title>\n"
    "<link href=\"/styles/redfish.css\" rel=\"stylesheet\">\n"
    "</head>\n"
    "<body>\n"
    "<div class=\"container\">\n"
    "<img src=\"/images/DMTF_Redfish_logo_2017.svg\" alt=\"redfish\" height=\"406px\" width=\"576px\">\n";

const std::string boilerplateEnd = "</div>\n"
                                   "</body>\n"
                                   "</html>\n";

TEST(JsonHtmlSerializer, dumpHtmlLink)
{
    std::string out;
    nlohmann::json j;
    j["@odata.id"] = "/redfish/v1";
    dumpHtml(out, j);
    EXPECT_EQ(
        out,
        boilerplateStart +
            "<div class=\"content\">\n"
            "{<div class=tab>&quot@odata.id&quot: <a href=\"/redfish/v1\">\"/redfish/v1\"</a><br></div>}</div>\n" +
            boilerplateEnd);
}

TEST(JsonHtmlSerializer, dumpint)
{
    std::string out;
    nlohmann::json j = 42;
    dumpHtml(out, j);
    EXPECT_EQ(out, boilerplateStart + "<div class=\"content\">\n42</div>\n" +
                       boilerplateEnd);
}

TEST(JsonHtmlSerializer, dumpstring)
{
    std::string out;
    nlohmann::json j = "foobar";
    dumpHtml(out, j);
    EXPECT_EQ(out, boilerplateStart +
                       "<div class=\"content\">\n\"foobar\"</div>\n" +
                       boilerplateEnd);
}
} // namespace
} // namespace json_html_util
