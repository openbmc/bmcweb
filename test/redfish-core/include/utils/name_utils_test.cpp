#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "utils/name_utils.hpp"

#include <boost/system/errc.hpp>
#include <boost/system/error_code.hpp>

#include <memory>

#include <gtest/gtest.h>

namespace redfish::name_utils
{
namespace
{

TEST(NameUtils, PrettyNameOverridesDefault)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    asyncResp->res.jsonValue["Name"] = "Default Name";

    boost::system::error_code ec;
    afterGetPrettyName(asyncResp, ""_json_pointer, ec, "Pretty Name");

    EXPECT_EQ(asyncResp->res.jsonValue["Name"], "Pretty Name");
}

TEST(NameUtils, EmptyPrettyNamePreservesDefault)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    asyncResp->res.jsonValue["Name"] = "Default Name";

    boost::system::error_code ec;
    afterGetPrettyName(asyncResp, ""_json_pointer, ec, "");

    EXPECT_EQ(asyncResp->res.jsonValue["Name"], "Default Name");
}

TEST(NameUtils, ErrorPreservesDefault)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    asyncResp->res.jsonValue["Name"] = "Default Name";

    boost::system::error_code ec =
        boost::system::errc::make_error_code(boost::system::errc::io_error);

    afterGetPrettyName(asyncResp, ""_json_pointer, ec, "Pretty Name");

    EXPECT_EQ(asyncResp->res.jsonValue["Name"], "Default Name");
}

TEST(NameUtils, PrettyNameUsesJsonPointer)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    asyncResp->res.jsonValue["Nested"]["Name"] = "Default Name";

    boost::system::error_code ec;
    afterGetPrettyName(asyncResp, "/Nested"_json_pointer, ec, "Pretty Name");

    EXPECT_EQ(asyncResp->res.jsonValue["Nested"]["Name"], "Pretty Name");
}

TEST(NameUtils, MissingInventoryItemUsesDefault)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::MapperServiceMap services;

    getPrettyName(asyncResp, services, "/xyz/openbmc_project/inventory/test",
                  "Default Name");

    EXPECT_EQ(asyncResp->res.jsonValue["Name"], "Default Name");
}

} // namespace
} // namespace redfish::name_utils
