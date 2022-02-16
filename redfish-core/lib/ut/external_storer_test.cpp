#include "gtest/gtest.h"

#include "redfish-core/lib/external_storer.hpp"

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
        // Avoid having one test leak into the next test
        hookLogs->deleteAll();
    }
};

TEST_F(ExternalStorerTest, CreateInstance)
{
    auto a1 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleGetInstance(a1, "MyInstance");
    EXPECT_EQ(a1->res.result(), boost::beast::http::status::not_found);

    boost::beast::http::request<boost::beast::http::string_body> upBody;
    auto upJson = nlohmann::json::object();
    upJson["Id"] = "MyInstance";
    upBody.body() = upJson.dump();

    std::error_code ec;
    crow::Request req{upBody, ec};
    auto a2 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleCreateInstance(req, a2);
    EXPECT_EQ(a2->res.result(), boost::beast::http::status::created);

    auto a3 = std::make_shared<bmcweb::AsyncResp>();
    hookLogs->handleGetInstance(a1, "MyInstance");
    EXPECT_EQ(a3->res.result(), boost::beast::http::status::ok);
}

TEST_F(ExternalStorerTest, CreateMiddle)
{
    // ###
}

TEST_F(ExternalStorerTest, CreateEntry)
{
    // ###
}

TEST_F(ExternalStorerTest, GetInstance)
{
    // ###
}

TEST_F(ExternalStorerTest, GetMiddle)
{
    // ###
}

TEST_F(ExternalStorerTest, GetEntry)
{
    // ###
}
