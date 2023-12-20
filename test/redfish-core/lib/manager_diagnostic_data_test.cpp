#include "async_resp.hpp"
#include "manager_diagnostic_data.hpp"

#include <nlohmann/json.hpp>

#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

constexpr const char* chassisId = "ChassisId";
constexpr const char* validChassisPath = "ChassisPath";

void assertManagerDiagnosticDataGet(crow::Response& res)
{
    nlohmann::json& json = res.jsonValue;
    EXPECT_EQ(json["@odata.type"],
              "#ManagerDiagnosticData.v1_2_0.ManagerDiagnosticData");
}

TEST(ManagerDiagnosticDataTest, ManagerDiagnosticDataBasicAttributes)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    std::error_code err;
    crow::Request request{{boost::beast::http::verb::get, "/whatever", 11},
                          err};
    asyncResp->res.setCompleteRequestHandler(
        std::bind_front(assertManagerDiagnosticDataGet));
    crow::App app;
    handleManagerDiagnosticDataGet(app, request, asyncResp);
}

} // namespace
} // namespace redfish
