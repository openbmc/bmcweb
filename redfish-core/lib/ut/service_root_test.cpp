#include "app.hpp"
#include "http_request.hpp"
#include "include/async_resp.hpp"
#include "nlohmann/json.hpp"
#include "redfish-core/lib/service_root.hpp"

#include <boost/shared_ptr.hpp>

#include <string>

#include "gmock/gmock.h"

using namespace redfish;

void assertServiceRootGet()
{
    // do some checks of something
    // "/redfish/v1/"
    // EXPECT_THAT(privileges.getActivePrivilegeNames(PrivilegeType::BASE),
    //            ::testing::UnorderedElementsAre("Login", "ConfigureManager"));
}
TEST(ServiceRootTest, ServiceRootConstructor)
{

    // create arguments used in handler call
    boost::beast::http::request<boost::beast::http::string_body> in;
    crow::Request req(in);
    crow::Response response(assertServiceRootGet);
    std::shared_ptr<bmcweb::AsyncResp> asyncResp_ptr(
        new bmcweb::AsyncResp(response));
    const bmcweb::AsyncResp asyncResp(response);

    // call handler
    handleServiceRootGet(req, asyncResp_ptr);
}
