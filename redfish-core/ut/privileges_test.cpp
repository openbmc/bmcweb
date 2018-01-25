#include "privileges.hpp"
#include <fstream>
#include <string>
#include "nlohmann/json.hpp"
#include "gmock/gmock.h"

using namespace redfish;

class PrivilegeTest : public testing::Test {
 protected:
  nlohmann::json privilegeRegistryMockJson;

  std::vector<std::string> expectedBasePrivileges{
      "Login", "ConfigureManager", "ConfigureUsers", "ConfigureComponents",
      "ConfigureSelf"};

  std::vector<std::string> expectedOEMPrivileges{"OEMRoot", "OEMDummy"};

  PrivilegeTest() {
    crow::logger::setLogLevel(crow::LogLevel::CRITICAL);

    std::ofstream privilegeRegistryMockOfstream("privilege_registry_mock.json");
    privilegeRegistryMockJson = nlohmann::json::parse(
        "{\
                  \"@Redfish.Copyright\": \"Dummy copyright\",\
                  \"@odata.type\": \"#PrivilegeRegistry.v1_0_0.PrivilegeRegistry\",\
                  \"Id\": \"Dummy id\",\
                  \"Name\": \"Dummy name\",\
                  \"PrivilegesUsed\": [\
                        \"Login\",\
                        \"ConfigureManager\",\
                        \"ConfigureUsers\",\
                        \"ConfigureComponents\",\
                        \"ConfigureSelf\"],\
                  \"OEMPrivilegesUsed\": [\
                        \"OEMRoot\",\
                        \"OEMDummy\"],\
                  \"Mappings\": [\
                  {\
                      \"Entity\": \"TestEntity\",\
                      \"OperationMap\": {\
                          \"GET\": [\
                              {\
                                  \"Privilege\": [\
                                      \"Login\"\
                                  ]\
                              }\
                          ],\
                          \"PATCH\": [\
                              {\
                                  \"Privilege\": [\
                                      \"ConfigureManager\"\
                                  ]\
                              },\
                              {\
                                  \"Privilege\": [\
                                      \"ConfigureUser\",\
                                      \"ConfigureDummy\",\
                                      \"OEMRoot\"\
                                  ]\
                              }\
                          ],\
                          \"POST\": [\
                              {\
                                  \"Privilege\": [\
                                      \"ConfigureManager\",\
                                      \"OEMDummy\"\
                                  ]\
                              }\
                          ],\
                          \"DELETE\": [\
                              {\
                                  \"Privilege\": [\
                                      \"ConfigureManager\"\
                                  ]\
                              }\
                          ]\
                      }\
                  },\
                  {\
                      \"Entity\": \"EntityWithNonStringPrivilege\",\
                      \"OperationMap\": {\
                          \"GET\": [\
                              {\
                                  \"Privilege\": [\"Login\"]\
                              }\
                          ],\
                          \"POST\": [\
                              {\
                                  \"Privilege\": [1]\
                              }\
                          ]\
                      }\
                  }\
                  ]\
            }");
    privilegeRegistryMockOfstream << std::setw(4) << privilegeRegistryMockJson
                                  << std::endl;
    privilegeRegistryMockOfstream.close();
  }

  virtual ~PrivilegeTest() { std::remove("privilege_registry_mock.json"); }

  void removeFieldFromRegistry(const std::string& field) {
    std::ifstream in("privilege_registry_mock.json");
    nlohmann::json tempJson = nlohmann::json::parse(in);
    in.close();

    tempJson.erase(field);

    std::ofstream out("privilege_registry_mock.json");
    out << std::setw(4) << tempJson << std::endl;
    out.close();
  }

  void removeFieldFromMappings(const std::string& field) {
    std::ifstream in("privilege_registry_mock.json");
    nlohmann::json tempJson = nlohmann::json::parse(in);
    in.close();

    tempJson.at("Mappings")[0].erase(field);

    std::ofstream out("privilege_registry_mock.json");
    out << std::setw(4) << tempJson << std::endl;
    out.close();
  }

  void clearArryInJson(const std::string& key) {
    std::ifstream in("privilege_registry_mock.json");
    nlohmann::json tempJson = nlohmann::json::parse(in);
    in.close();

    tempJson[key].clear();

    std::ofstream out("privilege_registry_mock.json");
    out << std::setw(4) << tempJson << std::endl;
    out.close();
  }

  template <typename T>
  void fillPrivilegeArray(const std::string& key,
                          const std::vector<T>& values) {
    std::ifstream in("privilege_registry_mock.json");
    nlohmann::json tempJson = nlohmann::json::parse(in);
    in.close();

    tempJson[key].clear();
    for (const auto& value : values) {
      tempJson[key].push_back(value);
    }

    std::ofstream out("privilege_registry_mock.json");
    out << std::setw(4) << tempJson << std::endl;
    out.close();
  }

  template <typename T>
  void addRequiredPrivilege(const T& value) {
    std::ifstream in("privilege_registry_mock.json");
    nlohmann::json tempJson = nlohmann::json::parse(in);
    in.close();

    tempJson["Mappings"][0]["OperationMap"]["GET"][0]["Privilege"].push_back(
        value);

    std::ofstream out("privilege_registry_mock.json");
    out << std::setw(4) << tempJson << std::endl;
    out.close();
  }

  bool isPrivilegeRegistryParsed(const EntityPrivileges& entityPrivileges) {
    auto userPrivileges = Privileges();
    userPrivileges.setSinglePrivilege("Login");
    // given the privileges_registry_mock.json, GET should be allowed with Login
    // if the file got parsed successfully
    return entityPrivileges.isMethodAllowedWithPrivileges(crow::HTTPMethod::GET,
                                                          userPrivileges);
  }
};

TEST_F(PrivilegeTest, PrivilegeRegistryJsonSuccessfullParse) {
  PrivilegeProvider privilegeProvider("privilege_registry_mock.json");
  auto entityPrivileges =
      privilegeProvider.getPrivilegesRequiredByEntity("", "foo.bar.TestEntity");
  EXPECT_TRUE(isPrivilegeRegistryParsed(entityPrivileges));
}

TEST_F(PrivilegeTest, PrivilegeRegistryJsonNotFound) {
  std::remove("privilege_registry_mock.json");
  PrivilegeProvider privilegeProvider("privilege_registry_mock.json");
  auto entityPrivileges =
      privilegeProvider.getPrivilegesRequiredByEntity("", "foo.bar.TestEntity");
  EXPECT_FALSE(isPrivilegeRegistryParsed(entityPrivileges));
}

TEST_F(PrivilegeTest, PrivilegeRegistryMissingCopyright) {
  removeFieldFromRegistry("@Redfish.Copyright");
  PrivilegeProvider privilegeProvider("privilege_registry_mock.json");
  auto entityPrivileges =
      privilegeProvider.getPrivilegesRequiredByEntity("", "foo.bar.TestEntity");
  EXPECT_FALSE(isPrivilegeRegistryParsed(entityPrivileges));
}

TEST_F(PrivilegeTest, PrivilegeRegistryMissingOdataType) {
  removeFieldFromRegistry("@odata.type");
  PrivilegeProvider privilegeProvider("privilege_registry_mock.json");
  auto entityPrivileges =
      privilegeProvider.getPrivilegesRequiredByEntity("", "foo.bar.TestEntity");
  EXPECT_FALSE(isPrivilegeRegistryParsed(entityPrivileges));
}

TEST_F(PrivilegeTest, PrivilegeRegistryMissingId) {
  removeFieldFromRegistry("Id");
  PrivilegeProvider privilegeProvider("privilege_registry_mock.json");
  auto entityPrivileges =
      privilegeProvider.getPrivilegesRequiredByEntity("", "foo.bar.TestEntity");
  EXPECT_FALSE(isPrivilegeRegistryParsed(entityPrivileges));
}

TEST_F(PrivilegeTest, PrivilegeRegistryMissingName) {
  removeFieldFromRegistry("Name");
  PrivilegeProvider privilegeProvider("privilege_registry_mock.json");
  auto entityPrivileges =
      privilegeProvider.getPrivilegesRequiredByEntity("", "foo.bar.TestEntity");
  EXPECT_FALSE(isPrivilegeRegistryParsed(entityPrivileges));
}

TEST_F(PrivilegeTest, PrivilegeRegistryMissingMappings) {
  removeFieldFromRegistry("Mappings");
  PrivilegeProvider privilegeProvider("privilege_registry_mock.json");
  auto entityPrivileges =
      privilegeProvider.getPrivilegesRequiredByEntity("", "foo.bar.TestEntity");
  EXPECT_FALSE(isPrivilegeRegistryParsed(entityPrivileges));
}

TEST_F(PrivilegeTest, PrivilegeRegistryMissingPrivilegesUsed) {
  removeFieldFromRegistry("PrivilegesUsed");
  PrivilegeProvider privilegeProvider("privilege_registry_mock.json");
  auto entityPrivileges =
      privilegeProvider.getPrivilegesRequiredByEntity("", "foo.bar.TestEntity");
  EXPECT_FALSE(isPrivilegeRegistryParsed(entityPrivileges));
}

TEST_F(PrivilegeTest, PrivilegeRegistryMissingOEMPrivilegesUsed) {
  removeFieldFromRegistry("OEMPrivilegesUsed");
  PrivilegeProvider privilegeProvider("privilege_registry_mock.json");
  auto entityPrivileges =
      privilegeProvider.getPrivilegesRequiredByEntity("", "foo.bar.TestEntity");
  EXPECT_FALSE(isPrivilegeRegistryParsed(entityPrivileges));
}

TEST_F(PrivilegeTest, PrivilegeRegistryMissingOperationMap) {
  removeFieldFromMappings("OperationMap");
  PrivilegeProvider privilegeProvider("privilege_registry_mock.json");

  auto entityPrivileges =
      privilegeProvider.getPrivilegesRequiredByEntity("", "foo.bar.TestEntity");
  EXPECT_FALSE(isPrivilegeRegistryParsed(entityPrivileges));
}

TEST_F(PrivilegeTest, PrivilegeRegistryPrivilegesUsedMayNotBeEmpty) {
  clearArryInJson("PrivilegesUsed");
  PrivilegeProvider privilegeProvider("privilege_registry_mock.json");
  auto entityPrivileges =
      privilegeProvider.getPrivilegesRequiredByEntity("", "foo.bar.TestEntity");
  EXPECT_FALSE(isPrivilegeRegistryParsed(entityPrivileges));
}

TEST_F(PrivilegeTest, PrivilegeRegistryOEMPrivilegesUsedMayByEmpty) {
  clearArryInJson("OEMPrivilegesUsed");
  PrivilegeProvider privilegeProvider("privilege_registry_mock.json");
  auto entityPrivileges =
      privilegeProvider.getPrivilegesRequiredByEntity("", "foo.bar.TestEntity");
  EXPECT_TRUE(isPrivilegeRegistryParsed(entityPrivileges));
}

TEST_F(PrivilegeTest, PrivilegeValuesMayOnlyBeStrings) {
  std::vector<int> privilegesUsed = {1, 3, 4};
  fillPrivilegeArray("PrivilegesUsed", privilegesUsed);

  PrivilegeProvider privilegeProvider("privilege_registry_mock.json");
  auto entityPrivileges =
      privilegeProvider.getPrivilegesRequiredByEntity("", "foo.bar.TestEntity");
  EXPECT_FALSE(isPrivilegeRegistryParsed(entityPrivileges));
}

TEST_F(PrivilegeTest, OnlyMaxNoOfBasePrivilegesGetsLoaded) {
  const std::string excessivePrivilege("ExcessivePrivilege");
  std::vector<std::string> privilegesUsed;

  for (int i = 0; i < MAX_PRIVILEGE_COUNT; i++) {
    privilegesUsed.push_back(std::to_string(i));
  }
  privilegesUsed.push_back(excessivePrivilege);

  fillPrivilegeArray("PrivilegesUsed", privilegesUsed);
  addRequiredPrivilege(excessivePrivilege);

  PrivilegeProvider privilegeProvider("privilege_registry_mock.json");

  Privileges privileges;
  privileges.setSinglePrivilege(excessivePrivilege);

  EXPECT_EQ(privileges.getBasePrivilegeBitset(), 0);
}

TEST_F(PrivilegeTest, OnlyMaxNoOfOEMPrivilegesGetsLoaded) {
  const std::string excessivePrivilege("ExcessivePrivilege");
  std::vector<std::string> privilegesUsed;

  for (int i = 0; i < MAX_PRIVILEGE_COUNT; i++) {
    privilegesUsed.push_back(std::to_string(i));
  }
  privilegesUsed.push_back(excessivePrivilege);

  fillPrivilegeArray("OEMPrivilegesUsed", privilegesUsed);
  addRequiredPrivilege(excessivePrivilege);

  PrivilegeProvider privilegeProvider("privilege_registry_mock.json");

  Privileges privileges;
  privileges.setSinglePrivilege(excessivePrivilege);

  EXPECT_EQ(privileges.getOEMPrivilegeBitset(), 0);
}

TEST_F(PrivilegeTest, LoadEntityPrivilegesForExistingEntity) {
  PrivilegeProvider privilegeProvider("privilege_registry_mock.json");
  auto entityPrivileges =
      privilegeProvider.getPrivilegesRequiredByEntity("", "foo.bar.TestEntity");
  EXPECT_TRUE(isPrivilegeRegistryParsed(entityPrivileges));
}

TEST_F(PrivilegeTest, LoadEntityPrivilegesForNonExistingEntity) {
  PrivilegeProvider privilegeProvider("privilege_registry_mock.json");
  auto entityPrivileges =
      privilegeProvider.getPrivilegesRequiredByEntity("", "foo.bar.NotExists");
  EXPECT_FALSE(isPrivilegeRegistryParsed(entityPrivileges));
}

TEST_F(PrivilegeTest, LoadEntityPrivilegesForEntityWithNonStringPrivilege) {
  PrivilegeProvider privilegeProvider("privilege_registry_mock.json");
  auto entityPrivileges = privilegeProvider.getPrivilegesRequiredByEntity(
      "", "foo.bar.EntityWithNonStringPrivilege");
  EXPECT_FALSE(isPrivilegeRegistryParsed(entityPrivileges));
}

TEST_F(PrivilegeTest, DefaultEntityPrivilegesDenyAccess) {
  auto entityPrivileges = EntityPrivileges();

  auto res =
      entityPrivileges.isMethodAllowedForUser(crow::HTTPMethod::GET, "user");
  EXPECT_FALSE(res);
}

TEST_F(PrivilegeTest, PrivilegeCheckForSingleCaseSuccess) {
  PrivilegeProvider privilegeProvider("privilege_registry_mock.json");

  auto entityPrivileges = EntityPrivileges();
  auto userPrivileges = Privileges();
  auto requiredPrivileges = Privileges();

  userPrivileges.setSinglePrivilege("Login");
  requiredPrivileges.setSinglePrivilege("Login");
  entityPrivileges.addPrivilegesRequiredByMethod(crow::HTTPMethod::GET,
                                                 requiredPrivileges);
  EXPECT_TRUE(entityPrivileges.isMethodAllowedWithPrivileges(
      crow::HTTPMethod::GET, userPrivileges));
}

TEST_F(PrivilegeTest, PrivilegeCheckForSingleCaseFailure) {
  PrivilegeProvider privilegeProvider("privilege_registry_mock.json");

  auto entityPrivileges = EntityPrivileges();
  auto userPrivileges = Privileges();
  auto requiredPrivileges = Privileges();

  userPrivileges.setSinglePrivilege("Login");
  requiredPrivileges.setSinglePrivilege("ConfigureManager");
  entityPrivileges.addPrivilegesRequiredByMethod(crow::HTTPMethod::GET,
                                                 requiredPrivileges);
  EXPECT_FALSE(entityPrivileges.isMethodAllowedWithPrivileges(
      crow::HTTPMethod::GET, userPrivileges));
}

TEST_F(PrivilegeTest, PrivilegeCheckForANDCaseSuccess) {
  PrivilegeProvider privilegeProvider("privilege_registry_mock.json");

  auto entityPrivileges = EntityPrivileges();
  auto userPrivileges = Privileges();
  auto requiredPrivileges = Privileges();

  userPrivileges.setSinglePrivilege("Login");
  userPrivileges.setSinglePrivilege("ConfigureManager");
  userPrivileges.setSinglePrivilege("OEMRoot");
  requiredPrivileges.setSinglePrivilege("Login");
  requiredPrivileges.setSinglePrivilege("ConfigureManager");
  requiredPrivileges.setSinglePrivilege("OEMRoot");
  entityPrivileges.addPrivilegesRequiredByMethod(crow::HTTPMethod::GET,
                                                 requiredPrivileges);
  EXPECT_TRUE(entityPrivileges.isMethodAllowedWithPrivileges(
      crow::HTTPMethod::GET, userPrivileges));
}

TEST_F(PrivilegeTest, PrivilegeCheckForANDCaseFailure) {
  PrivilegeProvider privilegeProvider("privilege_registry_mock.json");

  auto entityPrivileges = EntityPrivileges();
  auto userPrivileges = Privileges();
  auto requiredPrivileges = Privileges();

  userPrivileges.setSinglePrivilege("Login");
  userPrivileges.setSinglePrivilege("ConfigureUsers");
  requiredPrivileges.setSinglePrivilege("Login");
  requiredPrivileges.setSinglePrivilege("OEMDummy");
  requiredPrivileges.setSinglePrivilege("ConfigureUsers");
  entityPrivileges.addPrivilegesRequiredByMethod(crow::HTTPMethod::GET,
                                                 requiredPrivileges);
  EXPECT_FALSE(entityPrivileges.isMethodAllowedWithPrivileges(
      crow::HTTPMethod::GET, userPrivileges));
}

TEST_F(PrivilegeTest, PrivilegeCheckForORCaseSuccess) {
  PrivilegeProvider privilegeProvider("privilege_registry_mock.json");

  auto entityPrivileges = EntityPrivileges();
  auto userPrivileges = Privileges();
  auto requiredPrivileges = Privileges();

  userPrivileges.setSinglePrivilege("OEMRoot");
  requiredPrivileges.setSinglePrivilege("Login");
  entityPrivileges.addPrivilegesRequiredByMethod(crow::HTTPMethod::GET,
                                                 requiredPrivileges);
  requiredPrivileges = Privileges();
  requiredPrivileges.setSinglePrivilege("OEMRoot");
  entityPrivileges.addPrivilegesRequiredByMethod(crow::HTTPMethod::GET,
                                                 requiredPrivileges);
  EXPECT_TRUE(entityPrivileges.isMethodAllowedWithPrivileges(
      crow::HTTPMethod::GET, userPrivileges));
}

TEST_F(PrivilegeTest, PrivilegeCheckForORCaseFailure) {
  PrivilegeProvider privilegeProvider("privilege_registry_mock.json");

  auto entityPrivileges = EntityPrivileges();
  auto userPrivileges = Privileges();
  auto requiredPrivileges = Privileges();

  userPrivileges.setSinglePrivilege("ConfigureComponents");
  requiredPrivileges.setSinglePrivilege("Login");
  entityPrivileges.addPrivilegesRequiredByMethod(crow::HTTPMethod::GET,
                                                 requiredPrivileges);
  requiredPrivileges = Privileges();
  requiredPrivileges.setSinglePrivilege("ConfigureManager");
  entityPrivileges.addPrivilegesRequiredByMethod(crow::HTTPMethod::GET,
                                                 requiredPrivileges);
  EXPECT_FALSE(entityPrivileges.isMethodAllowedWithPrivileges(
      crow::HTTPMethod::GET, userPrivileges));
}

TEST_F(PrivilegeTest, DefaultPrivilegeBitsetsAreEmpty) {
  Privileges privileges;
  EXPECT_TRUE(privileges.getBasePrivilegeBitset() == 0);
  EXPECT_TRUE(privileges.getOEMPrivilegeBitset() == 0);
}

TEST_F(PrivilegeTest, UniqueBitsAssignedForAllPrivilegeNames) {
  PrivilegeProvider privilegeProvider("privilege_registry_mock.json");

  Privileges privileges;

  for (const auto& privilege : expectedBasePrivileges) {
    privileges.setSinglePrivilege(privilege);
  }

  for (const auto& privilege : expectedOEMPrivileges) {
    privileges.setSinglePrivilege(privilege);
  }

  EXPECT_EQ(privileges.getBasePrivilegeBitset().count(),
            expectedBasePrivileges.size());
  EXPECT_EQ(privileges.getOEMPrivilegeBitset().count(),
            expectedOEMPrivileges.size());
}

TEST_F(PrivilegeTest, GetActiveBasePrivilegeNames) {
  Privileges privileges;

  EXPECT_EQ(privileges.getActivePrivilegeNames(PrivilegeType::BASE),
            std::vector<std::string>());

  PrivilegeProvider privilegeProvider("privilege_registry_mock.json");

  for (const auto& privilege : expectedBasePrivileges) {
    privileges.setSinglePrivilege(privilege);
  }

  std::vector<std::string> activePrivileges =
      privileges.getActivePrivilegeNames(PrivilegeType::BASE);

  std::sort(expectedBasePrivileges.begin(), expectedBasePrivileges.end());
  std::sort(activePrivileges.begin(), activePrivileges.end());

  EXPECT_EQ(activePrivileges, expectedBasePrivileges);
}

TEST_F(PrivilegeTest, GetActiveOEMPrivilegeNames) {
  Privileges privileges;

  EXPECT_EQ(privileges.getActivePrivilegeNames(PrivilegeType::OEM),
            std::vector<std::string>());

  PrivilegeProvider privilegeProvider("privilege_registry_mock.json");

  for (const auto& privilege : expectedOEMPrivileges) {
    privileges.setSinglePrivilege(privilege);
  }

  std::vector<std::string> activePrivileges =
      privileges.getActivePrivilegeNames(PrivilegeType::OEM);

  std::sort(expectedOEMPrivileges.begin(), expectedOEMPrivileges.end());
  std::sort(activePrivileges.begin(), activePrivileges.end());

  EXPECT_EQ(activePrivileges, expectedOEMPrivileges);
}
