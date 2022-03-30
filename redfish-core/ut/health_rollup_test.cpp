#include <health.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

class HealthRollupMock : public redfish::HealthRollup
{
  public:
    HealthRollupMock(const std::string& objPathIn,
                     const uint64_t& dbusTimeoutUsIn = 100000,
                     const bool autoUpdate = true) :
        redfish::HealthRollup(objPathIn, dbusTimeoutUsIn, autoUpdate)
    {}
    MOCK_METHOD(std::string, getHealth, (const std::string&));
    MOCK_METHOD(std::shared_ptr<std::vector<std::string>>, getChildren,
                (const std::string&));
};

TEST(HealthRollupTest, NoChildValid)
{
    std::vector<std::string> rootChildren{};
    HealthRollupMock health("/", 100000, false);
    EXPECT_CALL(health, getHealth("/")).WillOnce(Return("OK"));
    EXPECT_CALL(health, getChildren("/"))
        .WillOnce(
            Return(std::make_shared<std::vector<std::string>>(rootChildren)));
    health.update();
    EXPECT_EQ(health.valid, true);
    EXPECT_STREQ(health.health.c_str(), "OK");
    EXPECT_STREQ(health.healthRollup.c_str(), "OK");

    EXPECT_CALL(health, getHealth("/")).WillOnce(Return("Warning"));
    EXPECT_CALL(health, getChildren("/"))
        .WillOnce(
            Return(std::make_shared<std::vector<std::string>>(rootChildren)));
    health.update();
    EXPECT_EQ(health.valid, true);
    EXPECT_STREQ(health.health.c_str(), "Warning");
    EXPECT_STREQ(health.healthRollup.c_str(), "Warning");

    EXPECT_CALL(health, getHealth("/")).WillOnce(Return("Critical"));
    EXPECT_CALL(health, getChildren("/"))
        .WillOnce(
            Return(std::make_shared<std::vector<std::string>>(rootChildren)));
    health.update();
    EXPECT_EQ(health.valid, true);
    EXPECT_STREQ(health.health.c_str(), "Critical");
    EXPECT_STREQ(health.healthRollup.c_str(), "Critical");
}

TEST(HealthRollupTest, WithChildrenValid)
{
    std::vector<std::string> rootChildren{"/a", "/b"};
    HealthRollupMock health("/", 100000, false);

    EXPECT_CALL(health, getHealth("/")).WillOnce(Return("OK"));
    EXPECT_CALL(health, getChildren("/"))
        .WillOnce(
            Return(std::make_shared<std::vector<std::string>>(rootChildren)));
    EXPECT_CALL(health, getHealth("/a")).WillOnce(Return("OK"));
    EXPECT_CALL(health, getHealth("/b")).WillOnce(Return("OK"));
    health.update();
    EXPECT_EQ(health.valid, true);
    EXPECT_STREQ(health.health.c_str(), "OK");
    EXPECT_STREQ(health.healthRollup.c_str(), "OK");

    EXPECT_CALL(health, getHealth("/")).WillOnce(Return("OK"));
    EXPECT_CALL(health, getChildren("/"))
        .WillOnce(
            Return(std::make_shared<std::vector<std::string>>(rootChildren)));
    EXPECT_CALL(health, getHealth("/a")).WillOnce(Return("Warning"));
    EXPECT_CALL(health, getHealth("/b")).WillOnce(Return("OK"));
    health.update();
    EXPECT_EQ(health.valid, true);
    EXPECT_STREQ(health.health.c_str(), "OK");
    EXPECT_STREQ(health.healthRollup.c_str(), "Warning");

    EXPECT_CALL(health, getHealth("/")).WillOnce(Return("OK"));
    EXPECT_CALL(health, getChildren("/"))
        .WillOnce(
            Return(std::make_shared<std::vector<std::string>>(rootChildren)));
    EXPECT_CALL(health, getHealth("/a")).WillOnce(Return("Warning"));
    EXPECT_CALL(health, getHealth("/b")).WillOnce(Return("Critical"));
    health.update();
    EXPECT_EQ(health.valid, true);
    EXPECT_STREQ(health.health.c_str(), "OK");
    EXPECT_STREQ(health.healthRollup.c_str(), "Critical");

    EXPECT_CALL(health, getHealth("/")).WillOnce(Return("Warning"));
    EXPECT_CALL(health, getChildren("/"))
        .WillOnce(
            Return(std::make_shared<std::vector<std::string>>(rootChildren)));
    EXPECT_CALL(health, getHealth("/a")).WillOnce(Return("OK"));
    EXPECT_CALL(health, getHealth("/b")).WillOnce(Return("OK"));
    health.update();
    EXPECT_EQ(health.valid, true);
    EXPECT_STREQ(health.health.c_str(), "Warning");
    EXPECT_STREQ(health.healthRollup.c_str(), "Warning");

    EXPECT_CALL(health, getHealth("/")).WillOnce(Return("Critical"));
    EXPECT_CALL(health, getChildren("/"))
        .WillOnce(
            Return(std::make_shared<std::vector<std::string>>(rootChildren)));
    EXPECT_CALL(health, getHealth("/a")).WillOnce(Return("Warning"));
    EXPECT_CALL(health, getHealth("/b")).WillOnce(Return("OK"));
    health.update();
    EXPECT_EQ(health.valid, true);
    EXPECT_STREQ(health.health.c_str(), "Critical");
    EXPECT_STREQ(health.healthRollup.c_str(), "Critical");
}

TEST(HealthRollupTest, Invalid)
{
    std::vector<std::string> rootChildren{"/a", "/b"};
    HealthRollupMock health("/", 100000, false);

    EXPECT_CALL(health, getHealth("/")).WillOnce(Return("OK"));
    health.update();
    EXPECT_EQ(health.valid, false);

    EXPECT_CALL(health, getHealth("/")).WillOnce(Return("OK"));
    EXPECT_CALL(health, getChildren("/")).WillOnce(Return(nullptr));
    health.update();
    EXPECT_EQ(health.valid, false);

    EXPECT_CALL(health, getHealth("/")).WillOnce(Return("OK"));
    EXPECT_CALL(health, getChildren("/"))
        .WillOnce(
            Return(std::make_shared<std::vector<std::string>>(rootChildren)));
    EXPECT_CALL(health, getHealth("/a")).WillOnce(Return("OK"));
    EXPECT_CALL(health, getHealth("/b")).WillOnce(Return(""));
    health.update();
    EXPECT_EQ(health.valid, false);
}