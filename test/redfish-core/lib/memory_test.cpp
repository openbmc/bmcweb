// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "generated/enums/memory.hpp"
#include "http_response.hpp"
#include "memory.hpp"

#include <boost/asio/error.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/system/linux_error.hpp>
#include <nlohmann/json.hpp>

#include <memory>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{
TEST(Memory, TranslateMemoryTypeToRedfish)
{
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.DDR"),
              memory::MemoryDeviceType::DDR);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.DDR2"),
              memory::MemoryDeviceType::DDR2);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.DDR3"),
              memory::MemoryDeviceType::DDR3);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.DDR4"),
              memory::MemoryDeviceType::DDR4);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType."
                  "DDR4E_SDRAM"),
              memory::MemoryDeviceType::DDR4E_SDRAM);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.DDR5"),
              memory::MemoryDeviceType::DDR5);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType."
                  "LPDDR4_SDRAM"),
              memory::MemoryDeviceType::LPDDR4_SDRAM);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType."
                  "LPDDR3_SDRAM"),
              memory::MemoryDeviceType::LPDDR3_SDRAM);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType."
                  "DDR2_SDRAM_FB_DIMM"),
              memory::MemoryDeviceType::DDR2_SDRAM_FB_DIMM);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType."
                  "DDR2_SDRAM_FB_DIMM_PROB"),
              memory::MemoryDeviceType::DDR2_SDRAM_FB_DIMM_PROBE);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType."
                  "DDR_SGRAM"),
              memory::MemoryDeviceType::DDR_SGRAM);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.ROM"),
              memory::MemoryDeviceType::ROM);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.SDRAM"),
              memory::MemoryDeviceType::SDRAM);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.EDO"),
              memory::MemoryDeviceType::EDO);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType."
                  "FastPageMode"),
              memory::MemoryDeviceType::FastPageMode);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType."
                  "PipelinedNibble"),
              memory::MemoryDeviceType::PipelinedNibble);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType."
                  "Logical"),
              memory::MemoryDeviceType::Logical);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.HBM"),
              memory::MemoryDeviceType::HBM);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.HBM2"),
              memory::MemoryDeviceType::HBM2);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType.HBM3"),
              memory::MemoryDeviceType::HBM3);
    EXPECT_EQ(translateMemoryTypeToRedfish(
                  "xyz.openbmc_project.Inventory.Item.Dimm.DeviceType."
                  "LPDDR5_SDRAM"),
              memory::MemoryDeviceType::Invalid);
    EXPECT_EQ(translateMemoryTypeToRedfish("invalid"),
              memory::MemoryDeviceType::Invalid);
}

TEST(AfterGetDimmChassisLink, ErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetDimmChassisLink(response, ec, {});

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetDimmChassisLink, EbadrOmitsChassis)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::linux_error::bad_request_descriptor;

    afterGetDimmChassisLink(response, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("Links"));
}

TEST(AfterGetDimmChassisLink, EmptyPathsOmitsChassis)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetDimmChassisLink(response, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("Links"));
}

TEST(AfterGetDimmChassisLink, MultipleChassisOmitChassis)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetSubTreePathsResponse chassisPaths = {
        "/xyz/openbmc_project/inventory/system/board/Chassis_0",
        "/xyz/openbmc_project/inventory/system/board/Chassis_1"};

    afterGetDimmChassisLink(response, ec, chassisPaths);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("Links"));
}

TEST(AfterGetDimmChassisLink, MalformedPathOmitsChassis)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetSubTreePathsResponse chassisPaths = {"/"};

    afterGetDimmChassisLink(response, ec, chassisPaths);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("Links"));
}

TEST(AfterGetDimmChassisLink, SuccessSetsChassisLink)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetSubTreePathsResponse chassisPaths = {
        "/xyz/openbmc_project/inventory/system/board/Chassis_0"};

    afterGetDimmChassisLink(response, ec, chassisPaths);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(response->res.jsonValue["Links"]["Chassis"]["@odata.id"],
              "/redfish/v1/Chassis/Chassis_0");
}

TEST(AfterGetDimmProcessorLinks, ErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetDimmProcessorLinks(response, ec, {});

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetDimmProcessorLinks, EbadrOmitsProcessors)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::linux_error::bad_request_descriptor;

    afterGetDimmProcessorLinks(response, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("Links"));
}

TEST(AfterGetDimmProcessorLinks, EmptyPathsOmitsProcessors)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetDimmProcessorLinks(response, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("Links"));
}

TEST(AfterGetDimmProcessorLinks, MultipleProcessorsSetProcessorLinks)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetSubTreePathsResponse processorPaths = {
        "/xyz/openbmc_project/inventory/GPU_0",
        "/xyz/openbmc_project/inventory/GPU_1"};

    afterGetDimmProcessorLinks(response, ec, processorPaths);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(response->res.jsonValue["Links"]["Processors@odata.count"], 2);
    EXPECT_EQ(response->res.jsonValue["Links"]["Processors"][0]["@odata.id"],
              "/redfish/v1/Systems/system/Processors/GPU_0");
    EXPECT_EQ(response->res.jsonValue["Links"]["Processors"][1]["@odata.id"],
              "/redfish/v1/Systems/system/Processors/GPU_1");
    EXPECT_FALSE(response->res.jsonValue["Links"].contains("Chassis"));
}

} // namespace
} // namespace redfish
