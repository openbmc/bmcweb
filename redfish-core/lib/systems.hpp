/*
// Copyright (c) 2018 Intel Corporation
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
*/
#pragma once

#include <error_messages.hpp>
#include <utils/json_utils.hpp>
#include "node.hpp"
#include "boost/container/flat_map.hpp"

namespace redfish {

/**
 * SystemAsyncResp
 * Gathers data needed for response processing after async calls are done
 */
class SystemAsyncResp {
 public:
  SystemAsyncResp(crow::Response &response) : res(response) {}

  ~SystemAsyncResp() {
    if (res.result() != (boost::beast::http::status::ok)) {
      // Reset the json object to clear out any data that made it in before the
      // error happened
      // todo(ed) handle error condition with proper code
      res.jsonValue = messages::internalError();
    }
    res.end();
  }

  void setErrorStatus() {
    res.result(boost::beast::http::status::internal_server_error);
  }

  crow::Response &res;
};

/**
 * OnDemandSystemsProvider
 * Board provider class that retrieves data directly from dbus, before seting
 * it into JSON output. This does not cache any data.
 *
 * Class can be a good example on how to scale different data providing
 * solutions to produce single schema output.
 *
 * TODO(Pawel)
 * This perhaps shall be different file, which has to be chosen on compile time
 * depending on OEM needs
 */
class OnDemandSystemsProvider {
 public:
  template <typename CallbackFunc>
  void getBaseboardList(CallbackFunc &&callback) {
    BMCWEB_LOG_DEBUG << "Get list of available boards.";
    crow::connections::systemBus->async_method_call(
        [callback{std::move(callback)}](const boost::system::error_code ec,
                                        const std::vector<std::string> &resp) {
          // Callback requires vector<string> to retrieve all available board
          // list.
          std::vector<std::string> board_list;
          if (ec) {
            // Something wrong on DBus, the error_code is not important at this
            // moment, just return success=false, and empty output. Since size
            // of vector may vary depending on information from Entity Manager,
            // and empty output could not be treated same way as error.
            callback(false, board_list);
            return;
          }
          BMCWEB_LOG_DEBUG << "Got " << resp.size() << " boards.";
          // Iterate over all retrieved ObjectPaths.
          for (const std::string &objpath : resp) {
            std::size_t last_pos = objpath.rfind("/");
            if (last_pos != std::string::npos) {
              board_list.emplace_back(objpath.substr(last_pos + 1));
            }
          }
          // Finally make a callback with useful data
          callback(true, board_list);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        "/xyz/openbmc_project/inventory", int32_t(0),
        std::array<const char *, 1>{
            "xyz.openbmc_project.Inventory.Item.Board"});
  };

  /**
   * @brief Retrieves computer system properties over dbus
   *
   * @param[in] aResp Shared pointer for completing asynchronous calls
   * @param[in] name  Computer system name from request
   *
   * @return None.
   */
  void getComputerSystem(std::shared_ptr<SystemAsyncResp> aResp,
                         const std::string &name) {
    const std::array<const char *, 5> interfaces = {
        "xyz.openbmc_project.Inventory.Decorator.Asset",
        "xyz.openbmc_project.Inventory.Item.Cpu",
        "xyz.openbmc_project.Inventory.Item.Dimm",
        "xyz.openbmc_project.Inventory.Item.System",
        "xyz.openbmc_project.Common.UUID",
    };
    BMCWEB_LOG_DEBUG << "Get available system components.";
    crow::connections::systemBus->async_method_call(
        [ name, aResp{std::move(aResp)} ](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>
                &subtree) {
          if (ec) {
            BMCWEB_LOG_DEBUG << "DBUS response error";
            aResp->setErrorStatus();
            return;
          }
          bool foundName = false;
          // Iterate over all retrieved ObjectPaths.
          for (const std::pair<std::string,
                               std::vector<std::pair<std::string,
                                                     std::vector<std::string>>>>
                   &object : subtree) {
            const std::string &path = object.first;
            BMCWEB_LOG_DEBUG << "Got path: " << path;
            const std::vector<std::pair<std::string, std::vector<std::string>>>
                &connectionNames = object.second;
            if (connectionNames.size() < 1) {
              continue;
            }
            // Check if computer system exist
            if (boost::ends_with(path, name)) {
              foundName = true;
              BMCWEB_LOG_DEBUG << "Found name: " << name;
              const std::string connectionName = connectionNames[0].first;
              crow::connections::systemBus->async_method_call(
                  [ aResp, name(std::string(name)) ](
                      const boost::system::error_code ec,
                      const std::vector<std::pair<std::string, VariantType>>
                          &propertiesList) {
                    if (ec) {
                      BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                      aResp->setErrorStatus();
                      return;
                    }
                    BMCWEB_LOG_DEBUG << "Got " << propertiesList.size()
                                   << "properties for system";
                    for (const std::pair<std::string, VariantType> &property :
                         propertiesList) {
                      const std::string *value =
                          mapbox::getPtr<const std::string>(property.second);
                      if (value != nullptr) {
                        aResp->res.jsonValue[property.first] = *value;
                      }
                    }
                    aResp->res.jsonValue["Name"] = name;
                    aResp->res.jsonValue["Id"] =
                        aResp->res.jsonValue["SerialNumber"];
                  },
                  connectionName, path, "org.freedesktop.DBus.Properties",
                  "GetAll", "xyz.openbmc_project.Inventory.Decorator.Asset");
            } else {
              // This is not system, so check if it's cpu, dimm, UUID or BiosVer
              for (auto const &s : connectionNames) {
                for (auto const &i : s.second) {
                  if (boost::ends_with(i, "Dimm")) {
                    BMCWEB_LOG_DEBUG << "Found Dimm, now get it properties.";
                    crow::connections::systemBus->async_method_call(
                        [&, aResp](const boost::system::error_code ec,
                                   const std::vector<std::pair<
                                       std::string, VariantType>> &properties) {
                          if (ec) {
                            BMCWEB_LOG_ERROR << "DBUS response error " << ec;
                            aResp->setErrorStatus();
                            return;
                          }
                          BMCWEB_LOG_DEBUG << "Got " << properties.size()
                                         << "Dimm properties.";
                          for (const auto &p : properties) {
                            if (p.first == "MemorySize") {
                              const std::string *value =
                                  mapbox::getPtr<const std::string>(p.second);
                              if ((value != nullptr) && (*value != "NULL")) {
                                // Remove units char
                                int32_t unitCoeff;
                                if (boost::ends_with(*value, "MB")) {
                                  unitCoeff = 1000;
                                } else if (boost::ends_with(*value, "KB")) {
                                  unitCoeff = 1000000;
                                } else {
                                  BMCWEB_LOG_ERROR << "Unsupported memory units";
                                  aResp->setErrorStatus();
                                  return;
                                }

                                auto memSize = boost::lexical_cast<int>(
                                    value->substr(0, value->length() - 2));
                                aResp->res.jsonValue["TotalSystemMemoryGiB"] +=
                                    memSize * unitCoeff;
                                aResp->res.jsonValue["MemorySummary"]["Status"]
                                                     ["State"] = "Enabled";
                              }
                            }
                          }
                        },
                        s.first, path, "org.freedesktop.DBus.Properties",
                        "GetAll", "xyz.openbmc_project.Inventory.Item.Dimm");
                  } else if (boost::ends_with(i, "Cpu")) {
                    BMCWEB_LOG_DEBUG << "Found Cpu, now get it properties.";
                    crow::connections::systemBus->async_method_call(
                        [&, aResp](const boost::system::error_code ec,
                                   const std::vector<std::pair<
                                       std::string, VariantType>> &properties) {
                          if (ec) {
                            BMCWEB_LOG_ERROR << "DBUS response error " << ec;
                            aResp->setErrorStatus();
                            return;
                          }
                          BMCWEB_LOG_DEBUG << "Got " << properties.size()
                                         << "Cpu properties.";
                          for (const auto &p : properties) {
                            if (p.first == "ProcessorFamily") {
                              const std::string *value =
                                  mapbox::getPtr<const std::string>(p.second);
                              if (value != nullptr) {
                                aResp->res
                                    .jsonValue["ProcessorSummary"]["Count"] =
                                    aResp->res
                                        .jsonValue["ProcessorSummary"]["Count"]
                                        .get<int>() +
                                    1;
                                aResp->res.jsonValue["ProcessorSummary"]
                                                     ["Status"]["State"] =
                                    "Enabled";
                                aResp->res
                                    .jsonValue["ProcessorSummary"]["Model"] =
                                    *value;
                              }
                            }
                          }
                        },
                        s.first, path, "org.freedesktop.DBus.Properties",
                        "GetAll", "xyz.openbmc_project.Inventory.Item.Cpu");
                  } else if (boost::ends_with(i, "UUID")) {
                    BMCWEB_LOG_DEBUG << "Found UUID, now get it properties.";
                    crow::connections::systemBus->async_method_call(
                        [aResp](const boost::system::error_code ec,
                                const std::vector<std::pair<
                                    std::string, VariantType>> &properties) {
                          if (ec) {
                            BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                            aResp->setErrorStatus();
                            return;
                          }
                          BMCWEB_LOG_DEBUG << "Got " << properties.size()
                                         << "UUID properties.";
                          for (const std::pair<std::string, VariantType> &p :
                               properties) {
                            if (p.first == "BIOSVer") {
                              const std::string *value =
                                  mapbox::getPtr<const std::string>(p.second);
                              if (value != nullptr) {
                                aResp->res.jsonValue["BiosVersion"] = *value;
                              }
                            }
                            if (p.first == "UUID") {
                              const std::string *value =
                                  mapbox::getPtr<const std::string>(p.second);
                              BMCWEB_LOG_DEBUG << "UUID = " << *value
                                             << " length " << value->length();
                              if (value != nullptr) {
                                // Workaround for to short return str in smbios
                                // demo app, 32 bytes are described by spec
                                if (value->length() > 0 &&
                                    value->length() < 32) {
                                  std::string correctedValue = *value;
                                  correctedValue.append(32 - value->length(),
                                                        '0');
                                  value = &correctedValue;
                                } else if (value->length() == 32) {
                                  aResp->res.jsonValue["UUID"] =
                                      value->substr(0, 8) + "-" +
                                      value->substr(8, 4) + "-" +
                                      value->substr(12, 4) + "-" +
                                      value->substr(16, 4) + "-" +
                                      value->substr(20, 12);
                                }
                              }
                            }
                          }
                        },
                        s.first, path, "org.freedesktop.DBus.Properties",
                        "GetAll", "xyz.openbmc_project.Common.UUID");
                  }
                }
              }
            }
          }
          if (foundName == false) {
            aResp->setErrorStatus();
          }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", int32_t(0), interfaces);
  }

  /**
   * @brief Retrieves identify led group properties over dbus
   *
   * @param[in] aResp     Shared pointer for completing asynchronous calls.
   * @param[in] callback  Callback for process retrieved data.
   *
   * @return None.
   */
  template <typename CallbackFunc>
  void getLedGroupIdentify(std::shared_ptr<SystemAsyncResp> aResp,
                           CallbackFunc &&callback) {
    BMCWEB_LOG_DEBUG << "Get led groups";
    crow::connections::systemBus->async_method_call(
        [
          aResp{std::move(aResp)}, &callback
        ](const boost::system::error_code &ec, const ManagedObjectsType &resp) {
          if (ec) {
            BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
            aResp->setErrorStatus();
            return;
          }
          BMCWEB_LOG_DEBUG << "Got " << resp.size() << "led group objects.";
          for (const auto &objPath : resp) {
            const std::string &path = objPath.first;
            if (path.rfind("enclosure_identify") != std::string::npos) {
              for (const auto &interface : objPath.second) {
                if (interface.first == "xyz.openbmc_project.Led.Group") {
                  for (const auto &property : interface.second) {
                    if (property.first == "Asserted") {
                      const bool *asserted =
                          mapbox::getPtr<const bool>(property.second);
                      if (nullptr != asserted) {
                        callback(*asserted, aResp);
                      } else {
                        callback(false, aResp);
                      }
                    }
                  }
                }
              }
            }
          }
        },
        "xyz.openbmc_project.LED.GroupManager",
        "/xyz/openbmc_project/led/groups", "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects");
  }

  template <typename CallbackFunc>
  void getLedIdentify(std::shared_ptr<SystemAsyncResp> aResp,
                      CallbackFunc &&callback) {
    BMCWEB_LOG_DEBUG << "Get identify led properties";
    crow::connections::systemBus->async_method_call(
        [ aResp{std::move(aResp)}, &callback ](
            const boost::system::error_code ec,
            const PropertiesType &properties) {
          if (ec) {
            BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
            aResp->setErrorStatus();
            return;
          }
          BMCWEB_LOG_DEBUG << "Got " << properties.size() << "led properties.";
          std::string output;
          for (const auto &property : properties) {
            if (property.first == "State") {
              const std::string *s =
                  mapbox::getPtr<std::string>(property.second);
              if (nullptr != s) {
                BMCWEB_LOG_DEBUG << "Identify Led State: " << *s;
                const auto pos = s->rfind('.');
                if (pos != std::string::npos) {
                  auto led = s->substr(pos + 1);
                  for (const std::pair<const char *, const char *> &p :
                       std::array<std::pair<const char *, const char *>, 3>{
                           {{"On", "Lit"},
                            {"Blink", "Blinking"},
                            {"Off", "Off"}}}) {
                    if (led == p.first) {
                      output = p.second;
                    }
                  }
                }
              }
            }
          }
          callback(output, aResp);
        },
        "xyz.openbmc_project.LED.Controller.identify",
        "/xyz/openbmc_project/led/physical/identify",
        "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.Led.Physical");
  }

  /**
   * @brief Retrieves host state properties over dbus
   *
   * @param[in] aResp     Shared pointer for completing asynchronous calls.
   *
   * @return None.
   */
  void getHostState(std::shared_ptr<SystemAsyncResp> aResp) {
    BMCWEB_LOG_DEBUG << "Get host information.";
    crow::connections::systemBus->async_method_call(
        [aResp{std::move(aResp)}](const boost::system::error_code ec,
                                  const PropertiesType &properties) {
          if (ec) {
            BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
            aResp->setErrorStatus();
            return;
          }
          BMCWEB_LOG_DEBUG << "Got " << properties.size() << "host properties.";
          for (const auto &property : properties) {
            if (property.first == "CurrentHostState") {
              const std::string *s =
                  mapbox::getPtr<const std::string>(property.second);
              BMCWEB_LOG_DEBUG << "Host state: " << *s;
              if (nullptr != s) {
                const auto pos = s->rfind('.');
                if (pos != std::string::npos) {
                  // Verify Host State
                  if (s->substr(pos + 1) == "Running") {
                    aResp->res.jsonValue["PowerState"] = "On";
                    aResp->res.jsonValue["Status"]["State"] = "Enabled";
                  } else {
                    aResp->res.jsonValue["PowerState"] = "Off";
                    aResp->res.jsonValue["Status"]["State"] = "Disabled";
                  }
                }
              }
            }
          }
        },
        "xyz.openbmc_project.State.Host", "/xyz/openbmc_project/state/host0",
        "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.State.Host");
  }
};

/**
 * SystemsCollection derived class for delivering ComputerSystems Collection
 * Schema
 */
class SystemsCollection : public Node {
 public:
  SystemsCollection(CrowApp &app) : Node(app, "/redfish/v1/Systems/") {
    Node::json["@odata.type"] =
        "#ComputerSystemCollection.ComputerSystemCollection";
    Node::json["@odata.id"] = "/redfish/v1/Systems";
    Node::json["@odata.context"] =
        "/redfish/v1/"
        "$metadata#ComputerSystemCollection.ComputerSystemCollection";
    Node::json["Name"] = "Computer System Collection";

    entityPrivileges = {
        {boost::beast::http::verb::get, {{"Login"}}},
        {boost::beast::http::verb::head, {{"Login"}}},
        {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
  }

 private:
  /**
   * Functions triggers appropriate requests on DBus
   */
  void doGet(crow::Response &res, const crow::Request &req,
             const std::vector<std::string> &params) override {
    // Get board list, and call the below callback for JSON preparation
    provider.getBaseboardList(
        [&](const bool &success, const std::vector<std::string> &output) {
          if (success) {
            // ... prepare json array with appropriate @odata.id links
            nlohmann::json boardArray = nlohmann::json::array();
            for (const std::string &board_item : output) {
              boardArray.push_back(
                  {{"@odata.id", "/redfish/v1/Systems/" + board_item}});
            }
            // Then attach members, count size and return,
            Node::json["Members"] = boardArray;
            Node::json["Members@odata.count"] = boardArray.size();
            res.jsonValue = Node::json;
          } else {
            // ... otherwise, return INTERNALL ERROR
            res.result(boost::beast::http::status::internal_server_error);
          }
          res.end();
        });
  }

  OnDemandSystemsProvider provider;
};

/**
 * Systems override class for delivering ComputerSystems Schema
 */
class Systems : public Node {
 public:
  /*
   * Default Constructor
   */
  Systems(CrowApp &app)
      : Node(app, "/redfish/v1/Systems/<str>/", std::string()) {
    Node::json["@odata.type"] = "#ComputerSystem.v1_3_0.ComputerSystem";
    Node::json["@odata.context"] =
        "/redfish/v1/$metadata#ComputerSystem.ComputerSystem";
    Node::json["SystemType"] = "Physical";
    Node::json["Description"] = "Computer System";
    Node::json["Boot"]["BootSourceOverrideEnabled"] =
        "Disabled";  // TODO(Dawid), get real boot data
    Node::json["Boot"]["BootSourceOverrideTarget"] =
        "None";  // TODO(Dawid), get real boot data
    Node::json["Boot"]["BootSourceOverrideMode"] =
        "Legacy";  // TODO(Dawid), get real boot data
    Node::json["Boot"]["BootSourceOverrideTarget@Redfish.AllowableValues"] = {
        "None",      "Pxe",       "Hdd", "Cd",
        "BiosSetup", "UefiShell", "Usb"};  // TODO(Dawid), get real boot data
    Node::json["ProcessorSummary"]["Count"] = int(0);
    Node::json["ProcessorSummary"]["Status"]["State"] = "Disabled";
    Node::json["MemorySummary"]["TotalSystemMemoryGiB"] = int(0);
    Node::json["MemorySummary"]["Status"]["State"] = "Disabled";

    entityPrivileges = {
        {boost::beast::http::verb::get, {{"Login"}}},
        {boost::beast::http::verb::head, {{"Login"}}},
        {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
  }

 private:
  OnDemandSystemsProvider provider;

  /**
   * Functions triggers appropriate requests on DBus
   */
  void doGet(crow::Response &res, const crow::Request &req,
             const std::vector<std::string> &params) override {
    // Check if there is required param, truly entering this shall be
    // impossible
    if (params.size() != 1) {
      res.result(boost::beast::http::status::internal_server_error);
      res.end();
      return;
    }

    const std::string &name = params[0];

    res.jsonValue = Node::json;
    res.jsonValue["@odata.id"] = "/redfish/v1/Systems/" + name;

    auto asyncResp = std::make_shared<SystemAsyncResp>(res);

    provider.getLedGroupIdentify(
        asyncResp, [&](const bool &asserted,
                       const std::shared_ptr<SystemAsyncResp> &aResp) {
          if (asserted) {
            // If led group is asserted, then another call is needed to
            // get led status
            provider.getLedIdentify(
                aResp, [](const std::string &ledStatus,
                          const std::shared_ptr<SystemAsyncResp> &aResp) {
                  if (!ledStatus.empty()) {
                    aResp->res.jsonValue["IndicatorLED"] = ledStatus;
                  }
                });
          } else {
            aResp->res.jsonValue["IndicatorLED"] = "Off";
          }
        });
    provider.getComputerSystem(asyncResp, name);
    provider.getHostState(asyncResp);
  }

  void doPatch(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override {
    // Check if there is required param, truly entering this shall be
    // impossible
    if (params.size() != 1) {
      res.result(boost::beast::http::status::internal_server_error);
      res.end();
      return;
    }
    // Parse JSON request body
    nlohmann::json patch;
    if (!json_util::processJsonFromRequest(res, req, patch)) {
      return;
    }
    // Find key with new led value
    const std::string &name = params[0];
    const std::string *reqLedState = nullptr;
    json_util::Result r = json_util::getString(
        "IndicatorLED", patch, reqLedState,
        static_cast<int>(json_util::MessageSetting::TYPE_ERROR) |
            static_cast<int>(json_util::MessageSetting::MISSING),
        res.jsonValue, std::string("/" + name + "/IndicatorLED"));
    if ((r != json_util::Result::SUCCESS) || (reqLedState == nullptr)) {
      res.result(boost::beast::http::status::bad_request);
      res.end();
      return;
    }
    // Verify key value
    std::string dbusLedState;
    for (const auto &p : boost::container::flat_map<const char *, const char *>{
             {"On", "Lit"}, {"Blink", "Blinking"}, {"Off", "Off"}}) {
      if (*reqLedState == p.second) {
        dbusLedState = p.first;
      }
    }

    // Update led status
    auto asyncResp = std::make_shared<SystemAsyncResp>(res);
    res.jsonValue = Node::json;
    res.jsonValue["@odata.id"] = "/redfish/v1/Systems/" + name;

    provider.getHostState(asyncResp);
    provider.getComputerSystem(asyncResp, name);

    if (dbusLedState.empty()) {
      messages::addMessageToJsonRoot(
          res.jsonValue,
          messages::propertyValueNotInList(*reqLedState, "IndicatorLED"));
    } else {
      // Update led group
      BMCWEB_LOG_DEBUG << "Update led group.";
      crow::connections::systemBus->async_method_call(
          [&, asyncResp{std::move(asyncResp)} ](
              const boost::system::error_code ec) {
            if (ec) {
              BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
              asyncResp->setErrorStatus();
              return;
            }
            BMCWEB_LOG_DEBUG << "Led group update done.";
          },
          "xyz.openbmc_project.LED.GroupManager",
          "/xyz/openbmc_project/led/groups/enclosure_identify",
          "org.freedesktop.DBus.Properties", "Set",
          "xyz.openbmc_project.Led.Group", "Asserted",
          sdbusplus::message::variant<bool>(
              (dbusLedState == "Off" ? false : true)));
      // Update identify led status
      BMCWEB_LOG_DEBUG << "Update led SoftwareInventoryCollection.";
      crow::connections::systemBus->async_method_call(
          [&, asyncResp{std::move(asyncResp)} ](
              const boost::system::error_code ec) {
            if (ec) {
              BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
              asyncResp->setErrorStatus();
              return;
            }
            BMCWEB_LOG_DEBUG << "Led state update done.";
            res.jsonValue["IndicatorLED"] = *reqLedState;
          },
          "xyz.openbmc_project.LED.Controller.identify",
          "/xyz/openbmc_project/led/physical/identify",
          "org.freedesktop.DBus.Properties", "Set",
          "xyz.openbmc_project.Led.Physical", "State",
          sdbusplus::message::variant<std::string>(
              "xyz.openbmc_project.Led.Physical.Action." + dbusLedState));
    }
  }
};
}  // namespace redfish
