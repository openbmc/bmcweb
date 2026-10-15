// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/processor.hpp"
#include "generated/enums/resource.hpp"
#include "http_response.hpp"
#include "processor_core.hpp"

#include <boost/asio/error.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/error_code.hpp>
#include <nlohmann/json.hpp>

#include <memory>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

constexpr const char* coreLink =
    "</redfish/v1/JsonSchemas/Processor/Processor.json>; rel=describedby";
constexpr const char* collectionLink =
    "</redfish/v1/JsonSchemas/ProcessorCollection/ProcessorCollection.json>; rel=describedby";

boost::system::error_code ioError()
{
    return boost::system::errc::make_error_code(boost::system::errc::io_error);
}

dbus::utility::MapperGetSubTreeResponse coreSubTree()
{
    return {
        {"/xyz/openbmc_project/inventory/system/cpu0/core0",
         {{"xyz.openbmc_project.Example", {"xyz.openbmc_project.Example"}}}},
        {"/xyz/openbmc_project/inventory/system/cpu0/core1",
         {{"xyz.openbmc_project.Example", {"xyz.openbmc_project.Example"}}}}};
}

TEST(DoHandleSubProcessorCoreGet, IoErrorReturnsProcessorNotFound)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    doHandleSubProcessorCoreGet(asyncResp, "system", "cpu0", "core0", ioError(),
                                {});

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::not_found);
    EXPECT_EQ(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"][0],
              messages::resourceNotFound("Processor", "cpu0"));
}

TEST(DoHandleSubProcessorCoreGet, OtherErrorReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    doHandleSubProcessorCoreGet(asyncResp, "system", "cpu0", "core0",
                                boost::asio::error::invalid_argument, {});

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_EQ(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"][0],
              messages::internalError());
}

TEST(DoHandleSubProcessorCoreGet, MissingCoreReturnsNotFound)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    doHandleSubProcessorCoreGet(asyncResp, "system", "cpu0", "core9", {},
                                coreSubTree());

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::not_found);
    EXPECT_EQ(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"][0],
              messages::resourceNotFound("Processor", "core9"));
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("@odata.id"));
}

TEST(DoHandleSubProcessorCoreGet, MatchingCorePopulatesResource)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    doHandleSubProcessorCoreGet(asyncResp, "system", "cpu0", "core1", {},
                                coreSubTree());

    const nlohmann::json& json = asyncResp->res.jsonValue;
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(asyncResp->res.getHeaderValue(boost::beast::http::field::link),
              coreLink);
    EXPECT_EQ(json.size(), 6);
    EXPECT_EQ(json["@odata.type"], "#Processor.v1_18_0.Processor");
    EXPECT_EQ(json["@odata.id"],
              "/redfish/v1/Systems/system/Processors/cpu0/SubProcessors/core1");
    EXPECT_EQ(json["Id"], "core1");
    EXPECT_EQ(json["ProcessorType"], processor::ProcessorType::Core);
    EXPECT_EQ(json["Name"], "SubProcessor");
    EXPECT_EQ(json["Status"].size(), 2);
    EXPECT_EQ(json["Status"]["State"], resource::State::Enabled);
    EXPECT_EQ(json["Status"]["Health"], resource::Health::OK);
}

TEST(DoHandleSubProcessorCoreHead, IoErrorReturnsProcessorNotFound)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    doHandleSubProcessorCoreHead(asyncResp, "cpu0", "core0", ioError(), {});

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::not_found);
    EXPECT_EQ(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"][0],
              messages::resourceNotFound("Processor", "cpu0"));
}

TEST(DoHandleSubProcessorCoreHead, OtherErrorReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    doHandleSubProcessorCoreHead(asyncResp, "cpu0", "core0",
                                 boost::asio::error::invalid_argument, {});

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_EQ(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"][0],
              messages::internalError());
}

TEST(DoHandleSubProcessorCoreHead, MissingCoreReturnsNotFound)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    doHandleSubProcessorCoreHead(asyncResp, "cpu0", "core9", {}, coreSubTree());

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::not_found);
    EXPECT_EQ(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"][0],
              messages::resourceNotFound("Processor", "core9"));
    EXPECT_TRUE(
        asyncResp->res.getHeaderValue(boost::beast::http::field::link).empty());
}

TEST(DoHandleSubProcessorCoreHead, MatchingCoreSetsLinkHeader)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    doHandleSubProcessorCoreHead(asyncResp, "cpu0", "core0", {}, coreSubTree());

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(asyncResp->res.getHeaderValue(boost::beast::http::field::link),
              coreLink);
    EXPECT_TRUE(asyncResp->res.jsonValue.empty());
}

TEST(DoHandleSubProcessorCollectionGet, IoErrorReturnsProcessorNotFound)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    doHandleSubProcessorCollectionGet(asyncResp, "system", "cpu0", ioError(),
                                      {});

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::not_found);
    EXPECT_EQ(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"][0],
              messages::resourceNotFound("Processor", "cpu0"));
}

TEST(DoHandleSubProcessorCollectionGet, OtherErrorReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    doHandleSubProcessorCollectionGet(asyncResp, "system", "cpu0",
                                      boost::asio::error::invalid_argument, {});

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_EQ(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"][0],
              messages::internalError());
}

TEST(DoHandleSubProcessorCollectionGet, PathsPopulateSortedMembers)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::MapperGetSubTreePathsResponse paths = {
        "/xyz/openbmc_project/inventory/system/cpu0/core1",
        "/xyz/openbmc_project/inventory/system/cpu0/core0"};

    doHandleSubProcessorCollectionGet(asyncResp, "system", "cpu0", {}, paths);

    const nlohmann::json& json = asyncResp->res.jsonValue;
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(asyncResp->res.getHeaderValue(boost::beast::http::field::link),
              collectionLink);
    EXPECT_EQ(json.size(), 5);
    EXPECT_EQ(json["@odata.type"], "#ProcessorCollection.ProcessorCollection");
    EXPECT_EQ(json["@odata.id"],
              "/redfish/v1/Systems/system/Processors/cpu0/SubProcessors");
    EXPECT_EQ(json["Name"], "SubProcessor Collection");
    ASSERT_TRUE(json["Members"].is_array());
    ASSERT_EQ(json["Members"].size(), 2);
    EXPECT_EQ(json["Members"][0]["@odata.id"],
              "/redfish/v1/Systems/system/Processors/cpu0/SubProcessors/core0");
    EXPECT_EQ(json["Members"][1]["@odata.id"],
              "/redfish/v1/Systems/system/Processors/cpu0/SubProcessors/core1");
    EXPECT_EQ(json["Members@odata.count"], 2);
}

TEST(DoHandleSubProcessorCollectionGet, NoPathsReturnsEmptyMembers)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    doHandleSubProcessorCollectionGet(asyncResp, "system", "cpu0", {}, {});

    const nlohmann::json& json = asyncResp->res.jsonValue;
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(asyncResp->res.getHeaderValue(boost::beast::http::field::link),
              collectionLink);
    EXPECT_EQ(json.size(), 5);
    EXPECT_EQ(json["@odata.type"], "#ProcessorCollection.ProcessorCollection");
    EXPECT_EQ(json["@odata.id"],
              "/redfish/v1/Systems/system/Processors/cpu0/SubProcessors");
    EXPECT_EQ(json["Name"], "SubProcessor Collection");
    ASSERT_TRUE(json["Members"].is_array());
    EXPECT_TRUE(json["Members"].empty());
    EXPECT_EQ(json["Members@odata.count"], 0);
}

TEST(DoHandleSubProcessorCollectionHead, IoErrorReturnsProcessorNotFound)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    doHandleSubProcessorCollectionHead(asyncResp, "cpu0", ioError(), {});

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::not_found);
    EXPECT_EQ(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"][0],
              messages::resourceNotFound("Processor", "cpu0"));
    EXPECT_TRUE(
        asyncResp->res.getHeaderValue(boost::beast::http::field::link).empty());
}

TEST(DoHandleSubProcessorCollectionHead, OtherErrorReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    doHandleSubProcessorCollectionHead(
        asyncResp, "cpu0", boost::asio::error::invalid_argument, {});

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_EQ(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"][0],
              messages::internalError());
}

TEST(DoHandleSubProcessorCollectionHead, SuccessSetsLinkHeader)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    doHandleSubProcessorCollectionHead(asyncResp, "cpu0", {}, {});

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(asyncResp->res.getHeaderValue(boost::beast::http::field::link),
              collectionLink);
    EXPECT_TRUE(asyncResp->res.jsonValue.empty());
}

} // namespace
} // namespace redfish
