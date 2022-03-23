#include "async_resp.hpp"
#include "http_request.hpp"
#include "query_parameters/only.hpp"

#include <boost/url/url_view.hpp>

#include <memory>

#include "gtest/gtest.h"

namespace redfish::query_parameters
{
namespace
{

using ::testing::Test;

class OnlyMemberQueryTest : public Test
{
  public:
    OnlyMemberQueryTest() :
        req({}, ec), async_response(std::make_shared<bmcweb::AsyncResp>())
    {}

  protected:
    std::error_code ec;
    crow::Request req;
    std::shared_ptr<bmcweb::AsyncResp> async_response;
};

TEST_F(OnlyMemberQueryTest, ParseReturnsNullptrWithoutOnly)
{
    boost::urls::url_view url =
        boost::urls::parse_uri("https://www.bmcweb.com/?excerpt").value();

    EXPECT_EQ(
        Parameter::parse<OnlyMemberQuery>(url.params(), async_response->res),
        nullptr);
}

TEST_F(OnlyMemberQueryTest, ParseReturnsBadRequestIfOnlyHasValue)
{
    boost::urls::url_view url =
        boost::urls::parse_uri("https://www.bmcweb.com/?only=123").value();

    async_response->res.setCompleteRequestHandler([](crow::Response& response) {
        EXPECT_EQ(response.result(), boost::beast::http::status::bad_request);
    });
    EXPECT_EQ(
        Parameter::parse<OnlyMemberQuery>(url.params(), async_response->res),
        nullptr);
}

TEST_F(OnlyMemberQueryTest, ParseOnSuccess)
{
    boost::urls::url_view url =
        boost::urls::parse_uri("https://www.bmcweb.com/?only").value();

    EXPECT_NE(
        Parameter::parse<OnlyMemberQuery>(url.params(), async_response->res),
        nullptr);
}

} // namespace
} // namespace redfish::query_parameters
