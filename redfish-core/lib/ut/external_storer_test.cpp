#include "redfish-core/lib/external_storer.hpp"

#include "gtest/gtest.h"

class ExternalStorerTest : public ::testing::Test
{
  protected:
    App app;
    std::shared_ptr<external_storer::Hook> hookLogs;

  public:
    ExternalStorerTest() :
        hookLogs(std::make_shared<external_storer::Hook>(
            external_storer::makeLogServices()))
    {
        // Use our own Hook object, as we need it later in destructor
        redfish::requestRoutesExternalStorerLogServices(app, hookLogs);

        // Customize directory to be non-systemwide for testing
        hookLogs->setPathPrefix("./ExternalStorer_Test");

        // Clean up partial results of any previous interrupted run
        hookLogs->deleteAll();
    }

    ~ExternalStorerTest() override
    {
        // Clean up after ourselves
        hookLogs->deleteAll();
    }

    ExternalStorerTest(const ExternalStorerTest& copy) = delete;
    ExternalStorerTest& operator=(const ExternalStorerTest& assign) = delete;
    ExternalStorerTest(ExternalStorerTest&& move) = delete;
    ExternalStorerTest& operator=(ExternalStorerTest&& assign) = delete;
};

TEST_F(ExternalStorerTest, CreateGetInstance)
{
    // Must not exist originally
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

    // Create instance
    auto resp2 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleCreateInstance(req, resp2);
    EXPECT_EQ(resp2->res.result(), boost::beast::http::status::created);

    // Must now exist
    auto resp3 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleGetInstance(resp3, "MyInstance");
    EXPECT_EQ(resp3->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(resp3->res.jsonValue["Layer"], "Outer");

    // Outer layer and inner layer must both be individually customizable
    auto resp4 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleGetMiddle(resp4, "MyInstance", "Entries");
    EXPECT_EQ(resp4->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(resp4->res.jsonValue["Layer"], "Inner");
}

TEST_F(ExternalStorerTest, CreateGetMiddle)
{
    // Must not exist initially
    auto resp1 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleGetInstance(resp1, "MyInstance");
    EXPECT_EQ(resp1->res.result(), boost::beast::http::status::not_found);

    boost::beast::http::request<boost::beast::http::string_body> upBody;
    std::error_code ec;
    auto upJson = nlohmann::json::object();

    upJson["Id"] = "MyInstance";
    upBody.body() = upJson.dump();
    crow::Request req2{upBody, ec};

    // Create instance
    auto resp2 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleCreateInstance(req2, resp2);
    EXPECT_EQ(resp2->res.result(), boost::beast::http::status::created);

    // Instance layer must have linkage to middle layer
    auto resp3 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleGetInstance(resp3, "MyInstance");
    EXPECT_EQ(resp3->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(resp3->res.jsonValue["Entries"]["@odata.id"],
              "/redfish/v1/Systems/system/LogServices/MyInstance/Entries");

    // Entry must not initially exist
    auto resp4 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleGetMiddle(resp4, "MyInstance", "Entries");
    EXPECT_EQ(resp4->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(resp4->res.jsonValue["Members@odata.count"], 0);

    upJson = nlohmann::json::object();
    upJson["Id"] = "EntryCreatedFromCreateMiddle";
    upJson["Hello"] = "There";
    upBody.body() = upJson.dump();
    crow::Request req5{upBody, ec};

    // Create entry, by using middle layer
    auto resp5 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleCreateMiddle(req5, resp5, "MyInstance");
    EXPECT_EQ(resp5->res.result(), boost::beast::http::status::created);

    // Created entry must now appear in array of entries
    auto resp6 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleGetMiddle(resp6, "MyInstance", "Entries");
    EXPECT_EQ(resp6->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(resp6->res.jsonValue["Members@odata.count"], 1);
    EXPECT_EQ(
        resp6->res.jsonValue["Members"][0]["@odata.id"],
        "/redfish/v1/Systems/system/LogServices/MyInstance/Entries/EntryCreatedFromCreateMiddle");

    // Entry must now exist
    auto resp7 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleGetEntry(resp7, "MyInstance", "Entries",
                             "EntryCreatedFromCreateMiddle");
    EXPECT_EQ(resp7->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(resp7->res.jsonValue["Hello"], "There");
}

TEST_F(ExternalStorerTest, CreateGetEntry)
{
    // Must not exist initially
    auto resp1 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleGetInstance(resp1, "MyInstance");
    EXPECT_EQ(resp1->res.result(), boost::beast::http::status::not_found);

    boost::beast::http::request<boost::beast::http::string_body> upBody;
    std::error_code ec;
    auto upJson = nlohmann::json::object();

    upJson["Id"] = "MyInstance";
    upBody.body() = upJson.dump();
    crow::Request req2{upBody, ec};

    // Create instance
    auto resp2 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleCreateInstance(req2, resp2);
    EXPECT_EQ(resp2->res.result(), boost::beast::http::status::created);

    // Instance layer must have linkage to middle layer
    auto resp3 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleGetInstance(resp3, "MyInstance");
    EXPECT_EQ(resp3->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(resp3->res.jsonValue["Entries"]["@odata.id"],
              "/redfish/v1/Systems/system/LogServices/MyInstance/Entries");

    // Entry must not initially exist
    auto resp4 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleGetMiddle(resp4, "MyInstance", "Entries");
    EXPECT_EQ(resp4->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(resp4->res.jsonValue["Members@odata.count"], 0);

    upJson = nlohmann::json::object();
    upJson["Id"] = "EntryCreatedFromCreateEntry";
    upJson["Hello"] = "There";
    upBody.body() = upJson.dump();
    crow::Request req5{upBody, ec};

    // Create entry, by using entry layer
    auto resp5 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleCreateEntry(req5, resp5, "MyInstance", "Entries");
    EXPECT_EQ(resp5->res.result(), boost::beast::http::status::created);

    upJson = nlohmann::json::object();
    upJson["Id"] = "AnotherEntry";
    upJson["Hello"] = "Again";
    upBody.body() = upJson.dump();
    crow::Request req6{upBody, ec};

    // Create another entry, also by using entry layer
    auto resp6 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleCreateEntry(req6, resp6, "MyInstance", "Entries");
    EXPECT_EQ(resp6->res.result(), boost::beast::http::status::created);

    // Both entries must now appear in array of entries
    auto resp7 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleGetMiddle(resp7, "MyInstance", "Entries");
    EXPECT_EQ(resp7->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(resp7->res.jsonValue["Members@odata.count"], 2);

    // Both entries must now exist and be distinct from one another
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
