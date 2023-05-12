#include "app.hpp"
#include "async_resp.hpp"
#include "chassis.hpp"
#include "http_request.hpp"
#include "http_response.hpp"

#include <boost/beast/core/string_type.hpp>
#include <boost/beast/http/message.hpp>
#include <nlohmann/json.hpp>

#include <system_error>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

void assertChassisResetActionInfoGet(const std::string& chassisId,
                                     crow::Response& res)
{
    EXPECT_EQ(res.jsonValue["@odata.type"], "#ActionInfo.v1_1_2.ActionInfo");
    EXPECT_EQ(res.jsonValue["@odata.id"],
              "/redfish/v1/Chassis/" + chassisId + "/ResetActionInfo");
    EXPECT_EQ(res.jsonValue["Name"], "Reset Action Info");

    EXPECT_EQ(res.jsonValue["Id"], "ResetActionInfo");

    nlohmann::json::array_t parameters;
    nlohmann::json::object_t parameter;
    parameter["Name"] = "ResetType";
    parameter["Required"] = true;
    parameter["DataType"] = "String";
    nlohmann::json::array_t allowed;
    allowed.emplace_back("PowerCycle");
    parameter["AllowableValues"] = std::move(allowed);
    parameters.emplace_back(std::move(parameter));

    EXPECT_EQ(res.jsonValue["Parameters"], parameters);
}

TEST(HandleChassisResetActionInfoGet, StaticAttributesAreExpected)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    std::error_code err;
    crow::Request request{{boost::beast::http::verb::get, "/whatever", 11},
                          err};

    std::string fakeChassis = "fakeChassis";
    response->res.setCompleteRequestHandler(
        std::bind_front(assertChassisResetActionInfoGet, fakeChassis));

    crow::App app;
    handleChassisResetActionInfoGet(app, request, response, fakeChassis);
}

} // namespace
} // namespace redfish
