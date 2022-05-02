#include "google/dbus_utils.hpp"
#include "google/google_service_root.hpp"
#include "http_request.hpp"
#include "nlohmann/json.hpp"

#include <async_resp.hpp>
#include <utils/collection.hpp>
#include <utils/hex_utils.hpp>

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
        arg.depth != other.depth || arg.subtree != other.subtree)
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

static const DbusMethodAddr
    kHothSendHostCommandAddr("xyz.openbmc_project.Control.Hoth",
                             "/xyz/openbmc_project/Control/Hoth/Foo",
                             hothInterface, "SendHostCommand");

static const std::vector<std::string> kTestRootOfTrustIds({"Foo", "Bar"});
class GoogleServiceTest : public ::testing::Test
{
  public:
    GoogleServiceTest() :
        mock_obj_mapper_(std::make_shared<MockObjectMapper>()),
        mock_rf_utils_(std::make_shared<MockRedfishUtilWrapper>()),
        mock_hoth_iface_(std::make_shared<MockHothInterface>())
    {
        for (const std::string& rotId : kTestRootOfTrustIds)
        {
            rotSubTree_.push_back({"/xyz/openbmc_project/Control/Hoth/" + rotId,
                                   {{"xyz.openbmc_project.Control.Hoth",
                                     {"xyz.openbmc_project.Control.Hoth"}}}});
        }
    }

  protected:
    std::shared_ptr<GoogleServiceAsyncResp> makeGoogleServiceResp()
    {
        return std::make_shared<GoogleServiceAsyncResp>(
            std::make_shared<bmcweb::AsyncResp>(), mock_obj_mapper_,
            mock_rf_utils_, mock_hoth_iface_);
    }

    void loadRoTSubTreeInMapperWithErrorCode(boost::system::errc::errc_t errc)
    {
        boost::system::error_code errorCode =
            boost::system::errc::make_error_code(errc);
        ObjectMapperGetSubTreeParams searchParams = {
            .depth = 0,
            .subtree = hothSearchPath,
            .interfaces = {hothInterface}};
        EXPECT_CALL(
            *mock_obj_mapper_,
            getSubTree(EqualsAddr(DbusMethodAddr(
                           "xyz.openbmc_project.ObjectMapper",
                           "/xyz/openbmc_project/object_mapper",
                           "xyz.openbmc_project.ObjectMapper", "GetSubTree")),
                       EqualsSubTreeParams(searchParams), ::testing::_))
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
        EXPECT_EQ(json["Name"], rotId);
        EXPECT_EQ(json["Actions"]["#RootOfTrust.SendCommand"]["target"],
                  "/google/v1/RootOfTrustCollection/" + rotId +
                      "/Actions/RootOfTrust.SendCommand");
        EXPECT_EQ(json["Location"]["PartLocation"]["ServiceLabel"], rotId);
        EXPECT_EQ(json["Location"]["PartLocation"]["LocationType"], "Embedded");
    }

    static std::vector<uint8_t> wordToBytes(const std::string& word)
    {
        std::vector<uint8_t> out;
        std::copy(word.begin(), word.end(), std::back_inserter(out));
        return out;
    }

    static void addSendCommandBody(const std::string& request,
                                   crow::Request& req)
    {
        std::string body = R"({"Command": ")";
        body += bytesToHexString(wordToBytes(request));
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

    std::shared_ptr<MockObjectMapper> mock_obj_mapper_;
    std::shared_ptr<MockRedfishUtilWrapper> mock_rf_utils_;
    std::shared_ptr<MockHothInterface> mock_hoth_iface_;

    crow::openbmc_mapper::GetSubTreeType rotSubTree_;
};

TEST_F(GoogleServiceTest, GoogleV1RootGet)
{
    std::error_code ec;
    crow::Request req({}, ec);
    auto serviceResp = makeGoogleServiceResp();

    getGoogleV1(req, serviceResp->asyncResp);
    validateServiceRootGet(serviceResp->asyncResp->res);
}

TEST_F(GoogleServiceTest, RootOfTrustCollectionGet)
{
    std::error_code ec;
    crow::Request req({}, ec);
    auto serviceResp = makeGoogleServiceResp();
    EXPECT_CALL(*mock_rf_utils_,
                populateCollectionMembers(
                    ::testing::Eq(std::ref(serviceResp->asyncResp)),
                    ::testing::Eq("/google/v1/RootOfTrustCollection"),
                    ::testing::ElementsAre(
                        ::testing::Eq("xyz.openbmc_project.Control.Hoth")),
                    ::testing::Eq("/xyz/openbmc_project")))
        .Times(1)
        .WillOnce(::testing::DoAll(
            ::testing::InvokeWithoutArgs([asyncResp(serviceResp->asyncResp)]() {
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

    getRootOfTrustCollection(req, serviceResp);
    validateRootOfTrustCollectionGet(serviceResp->asyncResp->res);
}

TEST_F(GoogleServiceTest, RootOfTrustGetSuccess)
{
    std::error_code ec;
    crow::Request req({}, ec);
    auto serviceResp = makeGoogleServiceResp();
    loadRoTSubTreeInMapperWithErrorCode(boost::system::errc::success);
    getRootOfTrust(req, serviceResp, "Foo");
    validateRootOfTrustGet(serviceResp->asyncResp->res, "Foo");
}

TEST_F(GoogleServiceTest, RootOfTrustGetInternalError)
{
    loadRoTSubTreeInMapperWithErrorCode(boost::system::errc::broken_pipe);

    std::error_code ec;
    crow::Request req({}, ec);
    auto serviceResp = makeGoogleServiceResp();
    getRootOfTrust(req, serviceResp, "Foo");
    EXPECT_THAT(serviceResp->asyncResp->res.jsonValue["error"]["code"],
                ::testing::EndsWith("InternalError"));
}

TEST_F(GoogleServiceTest, RootOfTrustGetNotFoundError)
{
    auto serviceResp = makeGoogleServiceResp();
    loadRoTSubTreeInMapperWithErrorCode(boost::system::errc::success);

    std::error_code ec;
    crow::Request req({}, ec);
    getRootOfTrust(req, serviceResp, "Baz");
    EXPECT_THAT(serviceResp->asyncResp->res.jsonValue["error"]["code"],
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
    auto serviceResp = makeGoogleServiceResp();
    addSendCommandBody(command, req);
    sendRoTCommand(req, serviceResp, "Foo");
    EXPECT_EQ(serviceResp->asyncResp->res.jsonValue["CommandResponse"],
              bytesToHexString(wordToBytes(response)));
}

TEST_F(GoogleServiceTest, RootOfTrustSendCommandBadInput)
{
    std::error_code ec;
    crow::Request req({}, ec);
    auto serviceResp = makeGoogleServiceResp();
    req.body = "{}";
    sendRoTCommand(req, serviceResp, "Foo");
    EXPECT_THAT(serviceResp->asyncResp->res.jsonValue["error"]["code"],
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
    auto serviceResp = makeGoogleServiceResp();
    addSendCommandBody(command, req);
    sendRoTCommand(req, serviceResp, "Foo");
    EXPECT_THAT(serviceResp->asyncResp->res.jsonValue["error"]["code"],
                ::testing::EndsWith("InternalError"));
}

} // namespace test
} // namespace google_api
} // namespace crow
