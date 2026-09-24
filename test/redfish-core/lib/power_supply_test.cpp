// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "power_supply.hpp"

#include <boost/beast/http/status.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <optional>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(PowerSupplyCollection, InvalidChassisReturnsNotFound)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    afterGetValidPowerSupplyCollectionChassisPath(asyncResp, "InvalidChassis",
                                                  std::nullopt);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::not_found);

    const nlohmann::json& extendedInfo =
        asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"];
    ASSERT_EQ(extendedInfo.size(), 1);
    EXPECT_EQ(extendedInfo[0]["MessageId"], "Base.1.19.ResourceNotFound");
    EXPECT_EQ(extendedInfo[0]["MessageArgs"],
              nlohmann::json::array({"Chassis", "InvalidChassis"}));
}

} // namespace
} // namespace redfish
