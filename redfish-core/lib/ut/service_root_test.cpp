#include "nlohmann/json.hpp"
#include "redfish-core/lib/service_root.hpp"
#include "app.hpp"
#include "include/async_resp.hpp"
#include "http_request.hpp"
#include <string>

#include "gmock/gmock.h"

using namespace redfish;

TEST(PrivilegeTest, PrivilegeConstructor)
{

   void assertServiceRootGet()
   {
       //do some checks of something
   }

   // create arguments used in hanlder call
   crow::Request req;
   crow::Response response(assertServiceRootGet);
   const bmcweb::AsyncResp asyncResp(response);
   // mocking needed

   // call handler
   handleServiceRootGet(req, asyncResp)


/* option 2 
    auto io = std::make_shared<boost::asio::io_context>();
    App app(io);
    crow::Request req;
    crow::Response response;
    const bmcweb::AsyncResp asyncResp(response);
    std::string none = "";
    requestRoutesServiceRoot(app)->handle(req, asyncResp, &none);
*/
/* option 3 test end to end?
    SimpleApp app;
    requestRoutesServiceRoot(app);
    Server<SimpleApp> server(&app, "127.0.0.1", 45451);
    auto _ = async(launch::async, [&] { server.run(); });
    std::string sendmsg = "GET /\r\n\r\n";
*/




    // "/redfish/v1/"
    //EXPECT_THAT(privileges.getActivePrivilegeNames(PrivilegeType::BASE),
    //            ::testing::UnorderedElementsAre("Login", "ConfigureManager"));
}

