#include "redfish-core/lib/external_storer.hpp"

#include "gtest/gtest.h"

class ExternalStorerTest : public ::testing::Test
{
  protected:
    App app;
    std::shared_ptr<external_storer::Hook> hookLogs;

    void SetUp() override
    {
        // Customize directory to be non-systemwide for testing
        external_storer::pathPrefix = "./ExternalStorer_Test";

        // Use the real setup path that the real bmcweb app uses
        redfish::requestRoutesExternalStorer(app);

        // Shorten, for ease of use during testing
        hookLogs = external_storer::hookLogServices;
    }

    void TearDown() override
    {
        // Avoid having this test leak into the next test
        hookLogs->deleteAll();
    }
};

TEST_F(ExternalStorerTest, CreateGetInstance)
{
    auto resp1 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleGetInstance(resp1, "MyInstance");
    EXPECT_EQ(resp1->res.result(), boost::beast::http::status::not_found);

    boost::beast::http::request<boost::beast::http::string_body> upBody;
    std::error_code ec;
    auto upJson = nlohmann::json::object();

    upJson["Id"] = "MyInstance";
    upJson["Layer"] = "Outer";
    upJson["Entries"] = nlohmann::json::object();
    upJson["Entries"]["Layer"] = "Inner";
    upBody.body() = upJson.dump();
    crow::Request req{upBody, ec};

    auto resp2 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleCreateInstance(req, resp2);
    EXPECT_EQ(resp2->res.result(), boost::beast::http::status::created);

    auto resp3 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleGetInstance(resp3, "MyInstance");
    EXPECT_EQ(resp3->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(resp3->res.jsonValue["Layer"], "Outer");

    auto resp4 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleGetMiddle(resp4, "MyInstance", "Entries");
    EXPECT_EQ(resp4->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(resp4->res.jsonValue["Layer"], "Inner");
}

TEST_F(ExternalStorerTest, CreateGetMiddle)
{
    auto resp1 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleGetInstance(resp1, "MyInstance");
    EXPECT_EQ(resp1->res.result(), boost::beast::http::status::not_found);

    boost::beast::http::request<boost::beast::http::string_body> upBody;
    std::error_code ec;
    auto upJson = nlohmann::json::object();

    upJson["Id"] = "MyInstance";
    upBody.body() = upJson.dump();
    crow::Request req2{upBody, ec};

    auto resp2 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleCreateInstance(req2, resp2);
    EXPECT_EQ(resp2->res.result(), boost::beast::http::status::created);

    auto resp3 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleGetInstance(resp3, "MyInstance");
    EXPECT_EQ(resp3->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(resp3->res.jsonValue["Entries"]["@odata.id"],
              "/redfish/v1/Systems/system/LogServices/MyInstance/Entries");

    auto resp4 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleGetMiddle(resp4, "MyInstance", "Entries");
    EXPECT_EQ(resp4->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(resp4->res.jsonValue["Members@odata.count"], 0);

    upJson = nlohmann::json::object();
    upJson["Id"] = "EntryCreatedFromCreateMiddle";
    upJson["Hello"] = "There";
    upBody.body() = upJson.dump();
    crow::Request req5{upBody, ec};

    auto resp5 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleCreateMiddle(req5, resp5, "MyInstance");
    EXPECT_EQ(resp5->res.result(), boost::beast::http::status::created);

    auto resp6 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleGetMiddle(resp6, "MyInstance", "Entries");
    EXPECT_EQ(resp6->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(resp6->res.jsonValue["Members@odata.count"], 1);
    EXPECT_EQ(
        resp6->res.jsonValue["Members"][0]["@odata.id"],
        "/redfish/v1/Systems/system/LogServices/MyInstance/Entries/EntryCreatedFromCreateMiddle");

    auto resp7 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleGetEntry(resp7, "MyInstance", "Entries",
                             "EntryCreatedFromCreateMiddle");
    EXPECT_EQ(resp7->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(resp7->res.jsonValue["Hello"], "There");
}

TEST_F(ExternalStorerTest, CreateGetEntry)
{
    auto resp1 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleGetInstance(resp1, "MyInstance");
    EXPECT_EQ(resp1->res.result(), boost::beast::http::status::not_found);

    boost::beast::http::request<boost::beast::http::string_body> upBody;
    std::error_code ec;
    auto upJson = nlohmann::json::object();

    upJson["Id"] = "MyInstance";
    upBody.body() = upJson.dump();
    crow::Request req2{upBody, ec};

    auto resp2 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleCreateInstance(req2, resp2);
    EXPECT_EQ(resp2->res.result(), boost::beast::http::status::created);

    auto resp3 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleGetInstance(resp3, "MyInstance");
    EXPECT_EQ(resp3->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(resp3->res.jsonValue["Entries"]["@odata.id"],
              "/redfish/v1/Systems/system/LogServices/MyInstance/Entries");

    auto resp4 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleGetMiddle(resp4, "MyInstance", "Entries");
    EXPECT_EQ(resp4->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(resp4->res.jsonValue["Members@odata.count"], 0);

    upJson = nlohmann::json::object();
    upJson["Id"] = "EntryCreatedFromCreateEntry";
    upJson["Hello"] = "There";
    upBody.body() = upJson.dump();
    crow::Request req5{upBody, ec};

    auto resp5 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleCreateEntry(req5, resp5, "MyInstance", "Entries");
    EXPECT_EQ(resp5->res.result(), boost::beast::http::status::created);

    upJson = nlohmann::json::object();
    upJson["Id"] = "AnotherEntry";
    upJson["Hello"] = "Again";
    upBody.body() = upJson.dump();
    crow::Request req6{upBody, ec};

    auto resp6 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleCreateEntry(req6, resp6, "MyInstance", "Entries");
    EXPECT_EQ(resp6->res.result(), boost::beast::http::status::created);

    auto resp7 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleGetMiddle(resp7, "MyInstance", "Entries");
    EXPECT_EQ(resp7->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(resp7->res.jsonValue["Members@odata.count"], 2);

    auto resp8 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleGetEntry(resp8, "MyInstance", "Entries",
                             "EntryCreatedFromCreateEntry");
    EXPECT_EQ(resp8->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(resp8->res.jsonValue["Hello"], "There");

    auto resp9 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleGetEntry(resp9, "MyInstance", "Entries", "AnotherEntry");
    EXPECT_EQ(resp9->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(resp9->res.jsonValue["Hello"], "Again");
}
