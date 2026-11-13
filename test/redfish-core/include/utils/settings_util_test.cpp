#include "dbus_utility.hpp"
#include "utils/settings_util.hpp"

#include <boost/asio/error.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/linux_error.hpp>
#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>

#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace redfish::settings_util
{
namespace
{

struct CallbackResult
{
    bool called = false;
    boost::system::error_code ec;
    std::optional<OnResetControl> control;
};

OnResetControlCallback recordInto(CallbackResult& result)
{
    return [&result](const boost::system::error_code& ec,
                     const std::optional<OnResetControl>& control) {
        result.called = true;
        result.ec = ec;
        result.control = control;
    };
}

const OnResetControl testControl{
    "xyz.openbmc_project.GpuSensor",
    "/xyz/openbmc_project/control/port/GPU_0/NVLink_3"};

TEST(AfterGetApplyTime, OnResetReturnsControl)
{
    CallbackResult result;
    afterGetApplyTime(
        testControl, recordInto(result), {},
        "xyz.openbmc_project.Control.Decorator.ApplyTime.ApplyTimes.OnReset");

    ASSERT_TRUE(result.called);
    EXPECT_FALSE(result.ec);
    ASSERT_TRUE(result.control);
    const OnResetControl control = result.control.value_or(OnResetControl{});
    EXPECT_EQ(control.service, testControl.service);
    EXPECT_EQ(control.path, testControl.path);
}

TEST(AfterGetApplyTime, ImmediateReturnsNoControl)
{
    CallbackResult result;
    afterGetApplyTime(
        testControl, recordInto(result), {},
        "xyz.openbmc_project.Control.Decorator.ApplyTime.ApplyTimes.Immediate");

    ASSERT_TRUE(result.called);
    EXPECT_FALSE(result.ec);
    EXPECT_FALSE(result.control);
}

TEST(AfterGetApplyTime, OtherErrorIsPassedOn)
{
    CallbackResult result;
    afterGetApplyTime(testControl, recordInto(result),
                      boost::asio::error::timed_out, "");

    ASSERT_TRUE(result.called);
    EXPECT_TRUE(result.ec);
    EXPECT_FALSE(result.control);
}

TEST(AfterGetControlSubtree, NoAssociationReturnsNoControl)
{
    CallbackResult result;
    afterGetControlSubtree(recordInto(result),
                           boost::system::linux_error::bad_request_descriptor,
                           {});

    ASSERT_TRUE(result.called);
    EXPECT_FALSE(result.ec);
    EXPECT_FALSE(result.control);
}

TEST(AfterGetControlSubtree, EmptySubtreeReturnsNoControl)
{
    CallbackResult result;
    afterGetControlSubtree(recordInto(result), {}, {});

    ASSERT_TRUE(result.called);
    EXPECT_FALSE(result.ec);
    EXPECT_FALSE(result.control);
}

TEST(AfterGetControlSubtree, MissingDecoratorReturnsNoControl)
{
    dbus::utility::MapperGetSubTreeResponse subtree{
        {"/xyz/openbmc_project/control/port/GPU_0/NVLink_3",
         {{"xyz.openbmc_project.GpuSensor",
           {"xyz.openbmc_project.Association.Definitions",
            "xyz.openbmc_project.Control.Port.LinkState"}}}}};

    CallbackResult result;
    afterGetControlSubtree(recordInto(result), {}, subtree);

    ASSERT_TRUE(result.called);
    EXPECT_FALSE(result.ec);
    EXPECT_FALSE(result.control);
}

TEST(AfterGetControlSubtree, SeveralControlsIsError)
{
    dbus::utility::MapperGetSubTreeResponse subtree{
        {"/xyz/openbmc_project/control/port/GPU_0/NVLink_3",
         {{"xyz.openbmc_project.GpuSensor",
           {"xyz.openbmc_project.Control.Port.LinkState"}}}},
        {"/xyz/openbmc_project/control/port/GPU_0/NVLink_4",
         {{"xyz.openbmc_project.GpuSensor",
           {"xyz.openbmc_project.Control.Port.LinkState"}}}}};

    CallbackResult result;
    afterGetControlSubtree(recordInto(result), {}, subtree);

    ASSERT_TRUE(result.called);
    EXPECT_TRUE(result.ec);
    EXPECT_FALSE(result.control);
}

TEST(AfterGetControlSubtree, OtherErrorIsPassedOn)
{
    CallbackResult result;
    afterGetControlSubtree(recordInto(result), boost::asio::error::timed_out,
                           {});

    ASSERT_TRUE(result.called);
    EXPECT_TRUE(result.ec);
    EXPECT_FALSE(result.control);
}

TEST(AddSettingsAnnotation, AddsSettingsObjectAndApplyTime)
{
    nlohmann::json resource;
    addSettingsAnnotation(
        resource,
        boost::urls::url(
            "/redfish/v1/Systems/system/Processors/GPU_0/Ports/NVLink_3/Settings"));

    const nlohmann::json& annotation = resource["@Redfish.Settings"];
    EXPECT_EQ(annotation["@odata.type"], "#Settings.v1_3_3.Settings");
    EXPECT_EQ(
        annotation["SettingsObject"]["@odata.id"],
        "/redfish/v1/Systems/system/Processors/GPU_0/Ports/NVLink_3/Settings");
    EXPECT_EQ(annotation["SupportedApplyTimes"],
              nlohmann::json::array({"OnReset"}));
}

} // namespace
} // namespace redfish::settings_util
