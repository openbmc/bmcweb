#include "app.hpp"
#include "async_resp.hpp"
#include "event_service_manager.hpp"
#include "health.hpp"
#include "log_services.hpp"

#include <nlohmann/json.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(LogServicesDumpServiceTest, LogServicesInvalidDumpServiceGetReturnsError)
{
    auto shareAsyncResp = std::make_shared<bmcweb::AsyncResp>();
    getDumpServiceInfo(shareAsyncResp, "Invalid");
    EXPECT_EQ(shareAsyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

} // namespace
} // namespace redfish
