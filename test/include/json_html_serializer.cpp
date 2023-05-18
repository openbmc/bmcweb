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

TEST(JsonHtmlSerializer, dumpHtmlLink)
{
    std::string out;
    nlohmann::json j;
    j["@odata.id"] = "/redfish/v1";
    dumpHtml(out, j);
    EXPECT_EQ(
        out,
        "<html>\n"
        "<head>\n"
        "<title>Redfish API</title>\n"
        "<link href=\"/redfish.css\" rel=\"stylesheet\">\n"
        "</head>\n"
        "<body>\n"
        "<div class=\"container\">\n"
        "<img src=\"/DMTF_Redfish_logo_2017.svg\" alt=\"redfish\" height=\"406px\" width=\"576px\">\n"
        "<div class=\"content\">\n"
        "{<div class=tab>&quot@odata.id&quot: <a href=\"/redfish/v1\">\"/redfish/v1\"</a><br></div>}</div>\n"
        "</div>\n"
        "</body>\n"
        "</html>\n");
}

TEST(JsonHtmlSerializer, dumpint)
{
    std::string out;
    nlohmann::json j = 42;
    dumpHtml(out, j);
    EXPECT_EQ(
        out,
        "<html>\n"
        "<head>\n"
        "<title>Redfish API</title>\n"
        "<link href=\"/redfish.css\" rel=\"stylesheet\">\n"
        "</head>\n"
        "<body>\n"
        "<div class=\"container\">\n"
        "<img src=\"/DMTF_Redfish_logo_2017.svg\" alt=\"redfish\" height=\"406px\" width=\"576px\">\n"
        "<div class=\"content\">\n42</div>\n"
        "</div>\n"
        "</body>\n"
        "</html>\n");
}
} // namespace
} // namespace json_html_util
