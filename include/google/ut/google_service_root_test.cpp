// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "google/dbus_utils.hpp"
#include "google/google_service_root.hpp"
#include "http_request.hpp"
#include "nlohmann/json.hpp"
#include "openbmc_dbus_rest.hpp"

#include <async_resp.hpp>
#include <boost/algorithm/hex.hpp>
#include <utils/collection.hpp>

#include <memory>
#include <string>

#include "gmock/gmock.h"

namespace crow
{
namespace google_api
{
namespace test
{

class MockObjectMapper : public crow::google_api::ObjectMapperInterface
{
  public:
    MOCK_METHOD(void, getSubTree,
                (const DbusMethodAddr&, const ObjectMapperGetSubTreeParams&,
                 const ObjectMapperSubTreeCallback&),
                (override));
};

class MockHothInterface : public crow::google_api::HothInterface
{
  public:
    MOCK_METHOD(void, sendHostCommand,
                (const DbusMethodAddr&, const std::vector<uint8_t>&,
                 const HothSendCommandCallback&),
                (override));
};

class MockRedfishUtilWrapper : public crow::google_api::RedfishUtilWrapper
{
  public:
    MOCK_METHOD(void, populateCollectionMembers,
                (const std::shared_ptr<bmcweb::AsyncResp>&, const std::string&,
                 const std::vector<const char*>&, const char*),
                (override));
};

MATCHER_P(EqualsAddr, addr, "EqualsAddr")
{
    return arg.objpath == addr.objpath && arg.service == addr.service &&
           strcmp(arg.iface, addr.iface) == 0 && arg.method == addr.method;
}

MATCHER_P(EqualsSubTreeParams, other, "EqualsSubTreeParams")
{
    if (arg.interfaces.size() != other.interfaces.size() ||
        arg.depth != other.depth || strcmp(arg.subtree, other.subtree) != 0)
    {
        return false;
    }
    for (size_t i = 0; i < arg.interfaces.size(); ++i)
    {
        if (strcmp(arg.interfaces[i], other.interfaces[i]) != 0)
        {
            return false;
        }
    }

    return true;
}

static const DbusMethodAddr kHothSendHostCommandAddr = {
    .service = "xyz.openbmc_project.Control.Hoth",
    .objpath = "/xyz/openbmc_project/Control/Hoth/Foo",
    .iface = kHothInterface,
    .method = "SendHostCommand"};

static const std::vector<std::string> kTestRootOfTrustIds({"Foo", "Bar"});
class GoogleServiceTest : public ::testing::Test
{
  public:
    GoogleServiceTest()
    {
        for (const std::string& rotId : kTestRootOfTrustIds)
        {
            rotSubTree_.push_back({"/xyz/openbmc_project/Control/Hoth/" + rotId,
                                   {{"xyz.openbmc_project.Control.Hoth",
                                     {"xyz.openbmc_project.Control.Hoth"}}}});
        }

        auto objMapper = std::make_shared<MockObjectMapper>();
        crow::google_api::objMapper = objMapper;
        mock_obj_mapper_ = objMapper.get();

        auto rfUtils = std::make_shared<MockRedfishUtilWrapper>();
        crow::google_api::rfUtils = rfUtils;
        mock_rf_utils_ = rfUtils.get();

        auto hothInterface = std::make_shared<MockHothInterface>();
        crow::google_api::hothInterface = hothInterface;
        mock_hoth_iface_ = hothInterface.get();
    }

  protected:
    void loadRoTSubTreeInMapperWithErrorCode(boost::system::errc::errc_t errc)
    {
        boost::system::error_code errorCode =
            boost::system::errc::make_error_code(errc);
        EXPECT_CALL(*mock_obj_mapper_,
                    getSubTree(EqualsAddr(kObjMapperGetSubTreeAddr),
                               EqualsSubTreeParams(kRotSubTreeSearchParams),
                               ::testing::_))
            .Times(1)
            .WillOnce(::testing::DoAll(
                testing::InvokeArgument<2>(errorCode, std::ref(rotSubTree_)),
                ::testing::Return()));
    }

    static void validateRootOfTrustCollectionGet(crow::Response& res)
    {
        nlohmann::json& json = res.jsonValue;
        EXPECT_EQ(json["@odata.id"], "/google/v1/RootOfTrustCollection");
        EXPECT_EQ(json["@odata.type"],
                  "#RootOfTrustCollection.RootOfTrustCollection");
        EXPECT_EQ(json["Members@odata.count"], kTestRootOfTrustIds.size());

        nlohmann::json& members = json["Members"];
        for (size_t i = 0; i < members.size(); ++i)
        {
            EXPECT_EQ(members[i]["@odata.id"],
                      "/google/v1/RootOfTrustCollection/" +
                          kTestRootOfTrustIds[i]);
        }
    }

    static void validateServiceRootGet(crow::Response& res)
    {
        nlohmann::json& json = res.jsonValue;
        EXPECT_EQ(json["@odata.id"], "/google/v1");
        EXPECT_EQ(json["@odata.type"],
                  "#GoogleServiceRoot.v1_0_0.GoogleServiceRoot");
        EXPECT_EQ(json["@odata.id"], "/google/v1");
        EXPECT_EQ(json["Id"], "Google Rest RootService");
        EXPECT_EQ(json["Name"], "Google Service Root");
        EXPECT_EQ(json["Version"], "1.0.0");
        EXPECT_EQ(json["RootOfTrustCollection"]["@odata.id"],
                  "/google/v1/RootOfTrustCollection");
    }

    static void validateRootOfTrustGet(crow::Response& res,
                                       const std::string& rotId)
    {
        nlohmann::json& json = res.jsonValue;
        EXPECT_EQ(json["@odata.type"], "#RootOfTrust.v1_0_0.RootOfTrust");
        EXPECT_EQ(json["@odata.id"],
                  "/google/v1/RootOfTrustCollection/" + rotId);
        EXPECT_EQ(json["Status"]["State"], "Enabled");
        EXPECT_EQ(json["Id"], rotId);
        EXPECT_EQ(json["Name"], "RootOfTrust-" + rotId);
        EXPECT_EQ(json["Actions"]["#RootOfTrust.SendCommand"]["target"],
                  "/google/v1/RootOfTrustCollection/" + rotId +
                      "/Actions/RootOfTrust.SendCommand");
        nlohmann::json& members = json["Links"]["ComponentsProtected"];
        EXPECT_EQ(members[0]["@odata.id"], "/google/v1");
        EXPECT_EQ(json["Links"]["ComponentsProtected@odata.count"], 1);
    }

    static std::vector<uint8_t> wordToBytes(const std::string& word)
    {
        std::vector<uint8_t> out;
        std::copy(word.begin(), word.end(), std::back_inserter(out));
        return out;
    }

    static std::string bytesToHex(const std::vector<uint8_t>& bytes)
    {
        std::string out;
        boost::algorithm::hex(bytes.begin(), bytes.end(),
                              std::back_inserter(out));
        return out;
    }

    static void addSendCommandBody(const std::string& request,
                                   crow::Request& req)
    {
        std::string body = R"({"Command": ")";
        body += bytesToHex(wordToBytes(request));
        body += "\"}";
        req.body = body;
    }

    void
        setupHostCommandResponseWithErrorCode(boost::system::errc::errc_t errc,
                                              const std::vector<uint8_t>& input,
                                              std::vector<uint8_t>& output)
    {
        boost::system::error_code success =
            boost::system::errc::make_error_code(errc);
        EXPECT_CALL(*mock_hoth_iface_,
                    sendHostCommand(EqualsAddr(kHothSendHostCommandAddr),
                                    ::testing::ElementsAreArray(input),
                                    ::testing::_))
            .Times(1)
            .WillOnce(
                ::testing::DoAll(testing::InvokeArgument<2>(success, output),
                                 ::testing::Return()));
    }

    MockObjectMapper* mock_obj_mapper_;
    MockRedfishUtilWrapper* mock_rf_utils_;
    MockHothInterface* mock_hoth_iface_;

    crow::openbmc_mapper::GetSubTreeType rotSubTree_;
};

TEST_F(GoogleServiceTest, GoogleV1RootGet)
{
    std::error_code ec;
    crow::Request req({}, ec);
    auto shareAsyncResp = std::make_shared<bmcweb::AsyncResp>();

    getGoogleV1(req, shareAsyncResp);
    validateServiceRootGet(shareAsyncResp->res);
}

TEST_F(GoogleServiceTest, RootOfTrustCollectionGet)
{
    std::error_code ec;
    crow::Request req({}, ec);
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    EXPECT_CALL(*mock_rf_utils_,
                populateCollectionMembers(
                    ::testing::Eq(std::ref(asyncResp)),
                    ::testing::Eq("/google/v1/RootOfTrustCollection"),
                    ::testing::ElementsAre(
                        ::testing::Eq("xyz.openbmc_project.Control.Hoth")),
                    ::testing::Eq("/xyz/openbmc_project")))
        .Times(1)
        .WillOnce(::testing::DoAll(::testing::InvokeWithoutArgs([asyncResp]() {
            nlohmann::json& members = asyncResp->res.jsonValue["Members"];
            members = nlohmann::json::array();
            for (const std::string& rotId : kTestRootOfTrustIds)
            {
                members.push_back(
                    {{"@odata.id",
                      "/google/v1/RootOfTrustCollection/" + rotId}});
            }
            asyncResp->res.jsonValue["Members@odata.count"] =
                kTestRootOfTrustIds.size();
        })));

    getRootOfTrustCollection(req, asyncResp);
    validateRootOfTrustCollectionGet(asyncResp->res);
}

TEST_F(GoogleServiceTest, RootOfTrustGetSuccess)
{
    std::error_code ec;
    crow::Request req({}, ec);
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    loadRoTSubTreeInMapperWithErrorCode(boost::system::errc::success);
    getRootOfTrust(req, asyncResp, "Foo");
    validateRootOfTrustGet(asyncResp->res, "Foo");
}

TEST_F(GoogleServiceTest, RootOfTrustGetInternalError)
{
    loadRoTSubTreeInMapperWithErrorCode(boost::system::errc::broken_pipe);

    std::error_code ec;
    crow::Request req({}, ec);
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    getRootOfTrust(req, asyncResp, "Foo");
    EXPECT_THAT(asyncResp->res.jsonValue["error"]["code"],
                ::testing::EndsWith("InternalError"));
}

TEST_F(GoogleServiceTest, RootOfTrustGetNotFoundError)
{
    loadRoTSubTreeInMapperWithErrorCode(boost::system::errc::success);

    std::error_code ec;
    crow::Request req({}, ec);
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    getRootOfTrust(req, asyncResp, "Baz");
    EXPECT_THAT(asyncResp->res.jsonValue["error"]["code"],
                ::testing::EndsWith("ResourceNotFound"));
}

TEST_F(GoogleServiceTest, RootOfTrustSendCommandSuccess)
{
    std::string command = "Hello!";
    std::string response = "World!";
    std::vector<uint8_t> input = wordToBytes(command);
    std::vector<uint8_t> output = wordToBytes(response);
    setupHostCommandResponseWithErrorCode(boost::system::errc::success, input,
                                          output);
    loadRoTSubTreeInMapperWithErrorCode(boost::system::errc::success);

    std::error_code ec;
    crow::Request req({}, ec);
    addSendCommandBody(command, req);
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    sendRoTCommand(req, asyncResp, "Foo");
    EXPECT_EQ(asyncResp->res.jsonValue["CommandResponse"],
              bytesToHex(wordToBytes(response)));
}

TEST_F(GoogleServiceTest, RootOfTrustSendCommandBadInput)
{
    loadRoTSubTreeInMapperWithErrorCode(boost::system::errc::success);

    std::error_code ec;
    crow::Request req({}, ec);
    req.body = "{}";
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    sendRoTCommand(req, asyncResp, "Foo");
    EXPECT_THAT(asyncResp->res.jsonValue["error"]["code"],
                ::testing::EndsWith("ActionParameterMissing"));
}

TEST_F(GoogleServiceTest, RootOfTrustSendCommandInternalError)
{
    std::string command = "Hello!";
    std::vector<uint8_t> input = wordToBytes(command);
    std::vector<uint8_t> output;
    setupHostCommandResponseWithErrorCode(boost::system::errc::broken_pipe,
                                          input, output);
    loadRoTSubTreeInMapperWithErrorCode(boost::system::errc::success);

    std::error_code ec;
    crow::Request req({}, ec);
    addSendCommandBody(command, req);
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    sendRoTCommand(req, asyncResp, "Foo");
    EXPECT_THAT(asyncResp->res.jsonValue["error"]["code"],
                ::testing::EndsWith("InternalError"));
}

} // namespace test
} // namespace google_api
} // namespace crow
