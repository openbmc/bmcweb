#include "async_resp.hpp"
#include "manager_diagnostic_data.hpp"

#include <boost/asio/error.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/system/linux_error.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <limits>
#include <memory>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

using json_pointer = nlohmann::json::json_pointer;

void testDataGetNoError(boost::system::error_code ec)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    setBytesProperty(asyncResp,
                     json_pointer("/MemoryStatistics/FreeStorageSpace"), ec, 0);
    EXPECT_TRUE(asyncResp->res.jsonValue.is_null());
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
}

TEST(ManagerDiagnosticDataTest, ManagerDataGetServerUnreachable)
{
    testDataGetNoError(boost::asio::error::basic_errors::host_unreachable);
}

TEST(ManagerDiagnosticDataTest, ManagerDataGetPathInvalid)
{
    testDataGetNoError(boost::system::linux_error::bad_request_descriptor);
}

void verifyError(bmcweb::Response& res)
{
    EXPECT_EQ(res.result(), boost::beast::http::status::internal_server_error);
    res.clear();
}

TEST(ManagerDiagnosticDataTest, ManagerDataGetFailure)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::operation_aborted;

    setBytesProperty(asyncResp,
                     json_pointer("/MemoryStatistics/FreeStorageSpace"), ec, 0);
    verifyError(asyncResp->res);
}

TEST(ManagerDiagnosticDataTest, ManagerDataGetNullPtr)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    setPercentProperty(
        asyncResp,
        nlohmann::json::json_pointer("/MemoryStatistics/FreeStorageSpace"), {},
        std::numeric_limits<double>::quiet_NaN());
    EXPECT_EQ(asyncResp->res.jsonValue["FreeStorageSpaceKiB"], nullptr);
}

TEST(ManagerDiagnosticDataTest, ManagerDataGetSuccess)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    setBytesProperty(asyncResp, json_pointer("/FreeStorageSpaceKiB"), {},
                     204800.0);
    EXPECT_EQ(asyncResp->res.jsonValue["FreeStorageSpaceKiB"].get<int64_t>(),
              200);
}

} // namespace
} // namespace redfish
