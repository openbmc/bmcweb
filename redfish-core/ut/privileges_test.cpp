#include "privileges.hpp"
#include <fstream>
#include <string>
#include "nlohmann/json.hpp"
#include "gmock/gmock.h"

using namespace redfish;

TEST(PrivilegeTest, PrivilegeConstructor) {
  Privileges privileges = {"Login", "ConfigureManager"};

  auto activePrivileges =
      privileges.getActivePrivilegeNames(PrivilegeType::BASE);
  std::vector<std::string> expectedPrivileges{"Login", "ConfigureManager"};

  std::sort(expectedPrivileges.begin(), expectedPrivileges.end());
  std::sort(activePrivileges.begin(), activePrivileges.end());

  EXPECT_EQ(expectedPrivileges, activePrivileges);
}

TEST(PrivilegeTest, PrivilegeCheckForNoPrivilegesRequired) {
  auto userPrivileges = Privileges{"Login"};
  OperationMap operationMap = {{crow::HTTPMethod::GET, {{}}}};
  auto entityPrivileges = EntityPrivileges(std::move(operationMap));

  EXPECT_TRUE(entityPrivileges.isMethodAllowedWithPrivileges(
      crow::HTTPMethod::GET, userPrivileges));
}

TEST(PrivilegeTest, PrivilegeCheckForSingleCaseSuccess) {
  auto userPrivileges = Privileges{"Login"};
  OperationMap operationMap = {{crow::HTTPMethod::GET, {{"Login"}}}};
  auto entityPrivileges = EntityPrivileges(std::move(operationMap));

  EXPECT_TRUE(entityPrivileges.isMethodAllowedWithPrivileges(
      crow::HTTPMethod::GET, userPrivileges));
}

TEST(PrivilegeTest, PrivilegeCheckForSingleCaseFailure) {
  auto userPrivileges = Privileges{"Login"};
  OperationMap operationMap = {{crow::HTTPMethod::GET, {{"ConfigureManager"}}}};
  auto entityPrivileges = EntityPrivileges(std::move(operationMap));

  EXPECT_FALSE(entityPrivileges.isMethodAllowedWithPrivileges(
      crow::HTTPMethod::GET, userPrivileges));
}

TEST(PrivilegeTest, PrivilegeCheckForANDCaseSuccess) {
  auto userPrivileges =
      Privileges{"Login", "ConfigureManager", "ConfigureSelf"};
  OperationMap operationMap = {
      {crow::HTTPMethod::GET,
       {{"Login", "ConfigureManager", "ConfigureSelf"}}}};
  auto entityPrivileges = EntityPrivileges(std::move(operationMap));

  EXPECT_TRUE(entityPrivileges.isMethodAllowedWithPrivileges(
      crow::HTTPMethod::GET, userPrivileges));
}

TEST(PrivilegeTest, PrivilegeCheckForANDCaseFailure) {
  auto userPrivileges = Privileges{"Login", "ConfigureManager"};
  OperationMap operationMap = {
      {crow::HTTPMethod::GET,
       {{"Login", "ConfigureManager", "ConfigureSelf"}}}};
  auto entityPrivileges = EntityPrivileges(std::move(operationMap));

  EXPECT_FALSE(entityPrivileges.isMethodAllowedWithPrivileges(
      crow::HTTPMethod::GET, userPrivileges));
}

TEST(PrivilegeTest, PrivilegeCheckForORCaseSuccess) {
  auto userPrivileges = Privileges{"ConfigureManager"};
  OperationMap operationMap = {
      {crow::HTTPMethod::GET, {{"Login"}, {"ConfigureManager"}}}};
  auto entityPrivileges = EntityPrivileges(std::move(operationMap));

  EXPECT_TRUE(entityPrivileges.isMethodAllowedWithPrivileges(
      crow::HTTPMethod::GET, userPrivileges));
}

TEST(PrivilegeTest, PrivilegeCheckForORCaseFailure) {
  auto userPrivileges = Privileges{"ConfigureComponents"};
  OperationMap operationMap = {
      {crow::HTTPMethod::GET, {{"Login"}, {"ConfigureManager"}}}};
  auto entityPrivileges = EntityPrivileges(std::move(operationMap));

  EXPECT_FALSE(entityPrivileges.isMethodAllowedWithPrivileges(
      crow::HTTPMethod::GET, userPrivileges));
}

TEST(PrivilegeTest, DefaultPrivilegeBitsetsAreEmpty) {
  Privileges privileges;
  EXPECT_TRUE(privileges.getBasePrivilegeBitset() == 0);
  EXPECT_TRUE(privileges.getOEMPrivilegeBitset() == 0);
}

TEST(PrivilegeTest, UniqueBitsAssignedForAllPrivilegeNames) {
  Privileges privileges;
  std::vector<std::string> expectedPrivileges{
      "Login", "ConfigureManager", "ConfigureUsers", "ConfigureComponents",
      "ConfigureSelf"};

  for (const auto& privilege : expectedPrivileges) {
    privileges.setSinglePrivilege(privilege);
  }

  EXPECT_EQ(privileges.getBasePrivilegeBitset().count(),
            expectedPrivileges.size());
}

TEST(PrivilegeTest, GetActivePrivilegeNames) {
  Privileges privileges;

  EXPECT_EQ(privileges.getActivePrivilegeNames(PrivilegeType::BASE),
            std::vector<std::string>());

  std::vector<std::string> expectedPrivileges{
      "Login", "ConfigureManager", "ConfigureUsers", "ConfigureComponents",
      "ConfigureSelf"};

  for (const auto& privilege : expectedPrivileges) {
    privileges.setSinglePrivilege(privilege);
  }

  std::vector<std::string> activePrivileges =
      privileges.getActivePrivilegeNames(PrivilegeType::BASE);

  std::sort(expectedPrivileges.begin(), expectedPrivileges.end());
  std::sort(activePrivileges.begin(), activePrivileges.end());

  EXPECT_EQ(activePrivileges, expectedPrivileges);
}

TEST(PrivilegeTest, PropertyOverrideConstructor) {
  OperationMap operationMap = {
      {crow::HTTPMethod::GET, {{"Login"}, {"ConfigureManager"}}}};
  PropertyOverride propertyOverride(std::move(operationMap),
                                    {"Password", "Id"});
}
