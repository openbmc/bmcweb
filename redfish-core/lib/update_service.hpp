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

#include "node.hpp"
#include <boost/container/flat_map.hpp>

namespace redfish {
static std::unique_ptr<sdbusplus::bus::match::match> fwUpdateMatcher;

class OnDemandSoftwareInventoryProvider {
 public:
  template <typename CallbackFunc>
  void getAllSoftwareInventoryObject(CallbackFunc &&callback) {
    crow::connections::systemBus->async_method_call(
        [callback{std::move(callback)}](
            const boost::system::error_code error_code,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>
                &subtree) {
          BMCWEB_LOG_DEBUG << "get all software inventory object callback...";
          if (error_code) {
            // Something wrong on DBus, the error_code is not important at this
            // moment, just return success=false, and empty output. Since size
            // of vector may vary depending on information from Entity Manager,
            // and empty output could not be treated same way as error.
            callback(false, subtree);
            return;
          }

          if (subtree.empty()) {
            BMCWEB_LOG_DEBUG << "subtree empty";
            callback(false, subtree);
          } else {
            BMCWEB_LOG_DEBUG << "subtree has something";
            callback(true, subtree);
          }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/software", int32_t(1),
        std::array<const char *, 1>{"xyz.openbmc_project.Software.Activation"});
  }
};

class UpdateService : public Node {
 public:
  UpdateService(CrowApp &app) : Node(app, "/redfish/v1/UpdateService/") {
    Node::json["@odata.type"] = "#UpdateService.v1_2_0.UpdateService";
    Node::json["@odata.id"] = "/redfish/v1/UpdateService";
    Node::json["@odata.context"] =
        "/redfish/v1/$metadata#UpdateService.UpdateService";
    Node::json["Id"] = "UpdateService";
    Node::json["Description"] = "Service for Software Update";
    Node::json["Name"] = "Update Service";
    Node::json["HttpPushUri"] = "/redfish/v1/UpdateService";
    Node::json["ServiceEnabled"] = true;  // UpdateService cannot be disabled
    Node::json["FirmwareInventory"] = {
        {"@odata.id", "/redfish/v1/UpdateService/FirmwareInventory"}};

    entityPrivileges = {
        {boost::beast::http::verb::get, {{"Login"}}},
        {boost::beast::http::verb::head, {{"Login"}}},
        {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
  }

 private:
  void doGet(crow::Response &res, const crow::Request &req,
             const std::vector<std::string> &params) override {
    res.jsonValue = Node::json;
    res.end();
  }
  static void activateImage(const std::string &obj_path) {
    crow::connections::systemBus->async_method_call(
        [obj_path](const boost::system::error_code error_code) {
          if (error_code) {
            BMCWEB_LOG_DEBUG << "error_code = " << error_code;
            BMCWEB_LOG_DEBUG << "error msg = " << error_code.message();
          }
        },
        "xyz.openbmc_project.Software.BMC.Updater", obj_path,
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Software.Activation", "RequestedActivation",
        sdbusplus::message::variant<std::string>(
            "xyz.openbmc_project.Software.Activation.RequestedActivations."
            "Active"));
  }
  void doPost(crow::Response &res, const crow::Request &req,
              const std::vector<std::string> &params) override {
    BMCWEB_LOG_DEBUG << "doPost...";

    // Only allow one FW update at a time
    if (fwUpdateMatcher != nullptr) {
      res.addHeader("Retry-After", "30");
      res.result(boost::beast::http::status::service_unavailable);
      res.jsonValue = messages::serviceTemporarilyUnavailable("3");
      res.end();
      return;
    }
    // Make this const static so it survives outside this method
    static boost::asio::deadline_timer timeout(*req.ioService,
                                               boost::posix_time::seconds(5));

    timeout.expires_from_now(boost::posix_time::seconds(5));

    timeout.async_wait([&res](const boost::system::error_code &ec) {
      fwUpdateMatcher = nullptr;
      if (ec == boost::asio::error::operation_aborted) {
        // expected, we were canceled before the timer completed.
        return;
      }
      BMCWEB_LOG_ERROR << "Timed out waiting for firmware object being created";
      BMCWEB_LOG_ERROR << "FW image may has already been uploaded to server";
      if (ec) {
        BMCWEB_LOG_ERROR << "Async_wait failed" << ec;
        return;
      }

      res.result(boost::beast::http::status::internal_server_error);
      res.jsonValue = redfish::messages::internalError();
      res.end();
    });

    auto callback = [&res](sdbusplus::message::message &m) {
      BMCWEB_LOG_DEBUG << "Match fired";
      bool flag = false;

      if (m.is_method_error()) {
        BMCWEB_LOG_DEBUG << "Dbus method error!!!";
        res.end();
        return;
      }
      std::vector<std::pair<
          std::string,
          std::vector<std::pair<std::string,
                                sdbusplus::message::variant<std::string>>>>>
          interfaces_properties;

      sdbusplus::message::object_path obj_path;

      m.read(obj_path, interfaces_properties);  // Read in the object path
                                                // that was just created
      // std::string str_objpath = obj_path.str;  // keep a copy for
      // constructing response message
      BMCWEB_LOG_DEBUG << "obj path = " << obj_path.str;  // str_objpath;
      for (auto &interface : interfaces_properties) {
        BMCWEB_LOG_DEBUG << "interface = " << interface.first;

        if (interface.first == "xyz.openbmc_project.Software.Activation") {
          // cancel timer only when xyz.openbmc_project.Software.Activation
          // interface is added
          boost::system::error_code ec;
          timeout.cancel(ec);
          if (ec) {
            BMCWEB_LOG_ERROR << "error canceling timer " << ec;
          }
          UpdateService::activateImage(obj_path.str);  // str_objpath);
          res.jsonValue = redfish::messages::success();
          BMCWEB_LOG_DEBUG << "ending response";
          res.end();
          fwUpdateMatcher = nullptr;
        }
      }
    };

    fwUpdateMatcher = std::make_unique<sdbusplus::bus::match::match>(
        *crow::connections::systemBus,
        "interface='org.freedesktop.DBus.ObjectManager',type='signal',"
        "member='InterfacesAdded',path='/xyz/openbmc_project/software'",
        callback);

    std::string filepath(
        "/tmp/images/" +
        boost::uuids::to_string(boost::uuids::random_generator()()));
    BMCWEB_LOG_DEBUG << "Writing file to " << filepath;
    std::ofstream out(filepath, std::ofstream::out | std::ofstream::binary |
                                    std::ofstream::trunc);
    out << req.body;
    out.close();
    BMCWEB_LOG_DEBUG << "file upload complete!!";
  }
};

class SoftwareInventoryCollection : public Node {
 public:
  /*
   * Default Constructor
   */
  template <typename CrowApp>
  SoftwareInventoryCollection(CrowApp &app)
      : Node(app, "/redfish/v1/UpdateService/FirmwareInventory/") {
    Node::json["@odata.type"] =
        "#SoftwareInventoryCollection.SoftwareInventoryCollection";
    Node::json["@odata.id"] = "/redfish/v1/UpdateService/FirmwareInventory";
    Node::json["@odata.context"] =
        "/redfish/v1/"
        "$metadata#SoftwareInventoryCollection.SoftwareInventoryCollection";
    Node::json["Name"] = "Software Inventory Collection";

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
    res.jsonValue = Node::json;
    softwareInventoryProvider.getAllSoftwareInventoryObject(
        [&](const bool &success,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>
                &subtree) {
          if (!success) {
            res.result(boost::beast::http::status::internal_server_error);
            res.jsonValue = messages::internalError();
            res.end();
            return;
          }

          if (subtree.empty()) {
            BMCWEB_LOG_DEBUG << "subtree empty!!";
            res.result(boost::beast::http::status::internal_server_error);
            res.jsonValue = messages::internalError();
            res.end();
            return;
          }

          res.jsonValue["Members"] = nlohmann::json::array();
          res.jsonValue["Members@odata.count"] = 0;

          std::shared_ptr<AsyncResp> asyncResp =
              std::make_shared<AsyncResp>(res);

          for (auto &obj : subtree) {
            const std::vector<std::pair<std::string, std::vector<std::string>>>
                &connections = obj.second;

            // if can't parse fw id then return
            std::size_t id_pos;
            if ((id_pos = obj.first.rfind("/")) == std::string::npos) {
              res.result(boost::beast::http::status::internal_server_error);
              res.jsonValue = messages::internalError();
              BMCWEB_LOG_DEBUG << "Can't parse firmware ID!!";
              res.end();
              return;
            }
            std::string fw_id = obj.first.substr(id_pos + 1);

            for (const auto &conn : connections) {
              const std::string connectionName = conn.first;
              BMCWEB_LOG_DEBUG << "connectionName = " << connectionName;
              BMCWEB_LOG_DEBUG << "obj.first = " << obj.first;

              crow::connections::systemBus->async_method_call(
                  [asyncResp, fw_id](
                      const boost::system::error_code error_code,
                      const sdbusplus::message::variant<std::string>
                          &activation_status) {
                    BMCWEB_LOG_DEBUG << "safe returned in lambda function";
                    if (error_code) {
                      asyncResp->res.result(
                          boost::beast::http::status::internal_server_error);
                      asyncResp->res.jsonValue = messages::internalError();
                      asyncResp->res.end();
                      return;
                    }
                    const std::string *activation_status_str =
                        mapbox::getPtr<const std::string>(activation_status);
                    if (activation_status_str != nullptr &&
                        *activation_status_str !=
                            "xyz.openbmc_project.Software.Activation."
                            "Activations.Active") {
                      // The activation status of this software is not currently
                      // active, so does not need to be listed in the response
                      return;
                    }
                    asyncResp->res.jsonValue["Members"].push_back(
                        {{"@odata.id",
                          "/redfish/v1/UpdateService/FirmwareInventory/" +
                              fw_id}});
                    asyncResp->res.jsonValue["Members@odata.count"] =
                        asyncResp->res.jsonValue["Members"].size();
                  },
                  connectionName, obj.first, "org.freedesktop.DBus.Properties",
                  "Get", "xyz.openbmc_project.Software.Activation",
                  "Activation");
            }
          }
        });
  }

  OnDemandSoftwareInventoryProvider softwareInventoryProvider;
};
/**
 * Chassis override class for delivering Chassis Schema
 */
class SoftwareInventory : public Node {
 public:
  /*
   * Default Constructor
   */
  template <typename CrowApp>
  SoftwareInventory(CrowApp &app)
      : Node(app, "/redfish/v1/UpdateService/FirmwareInventory/<str>/",
             std::string()) {
    Node::json["@odata.type"] = "#SoftwareInventory.v1_1_0.SoftwareInventory";
    Node::json["@odata.context"] =
        "/redfish/v1/$metadata#SoftwareInventory.SoftwareInventory";
    Node::json["Name"] = "Software Inventory";
    Node::json["Updateable"] = false;
    Node::json["Status"]["Health"] = "OK";
    Node::json["Status"]["HealthRollup"] = "OK";
    Node::json["Status"]["State"] = "Enabled";
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
    res.jsonValue = Node::json;

    if (params.size() != 1) {
      res.result(boost::beast::http::status::internal_server_error);
      res.jsonValue = messages::internalError();
      res.end();
      return;
    }

    const std::string &fw_id = params[0];
    res.jsonValue["Id"] = fw_id;
    res.jsonValue["@odata.id"] =
        "/redfish/v1/UpdateService/FirmwareInventory/" + fw_id;
    softwareInventoryProvider.getAllSoftwareInventoryObject([
      &res, id{std::string(fw_id)}
    ](const bool &success,
      const std::vector<std::pair<
          std::string,
          std::vector<std::pair<std::string, std::vector<std::string>>>>>
          &subtree) {
      BMCWEB_LOG_DEBUG << "doGet callback...";
      if (!success) {
        res.result(boost::beast::http::status::internal_server_error);
        res.jsonValue = messages::internalError();
        res.end();
        return;
      }

      if (subtree.empty()) {
        BMCWEB_LOG_ERROR << "subtree empty!!";
        res.result(boost::beast::http::status::not_found);
        res.end();
        return;
      }

      bool fw_id_found = false;

      for (auto &obj : subtree) {
        if (boost::ends_with(obj.first, id) != true) {
          continue;
        }
        fw_id_found = true;

        const std::vector<std::pair<std::string, std::vector<std::string>>>
            &connections = obj.second;

        if (connections.size() <= 0) {
          continue;
        }
        const std::pair<std::string, std::vector<std::string>> &conn =
            connections[0];
        const std::string &connectionName = conn.first;
        BMCWEB_LOG_DEBUG << "connectionName = " << connectionName;
        BMCWEB_LOG_DEBUG << "obj.first = " << obj.first;

        crow::connections::systemBus->async_method_call(
            [&res, id](
                const boost::system::error_code error_code,
                const boost::container::flat_map<std::string, VariantType>
                    &propertiesList) {
              if (error_code) {
                res.result(boost::beast::http::status::internal_server_error);
                res.jsonValue = messages::internalError();
                res.end();
                return;
              }

              boost::container::flat_map<std::string,
                                         VariantType>::const_iterator it =
                  propertiesList.find("Purpose");
              if (it == propertiesList.end()) {
                BMCWEB_LOG_ERROR << "Can't find property \"Purpose\"!";
                res.result(boost::beast::http::status::internal_server_error);
                res.jsonValue = messages::internalError();
                res.end();
                return;
              }

              // SoftwareId
              const std::string *sw_inv_purpose =
                  mapbox::getPtr<const std::string>(it->second);
              if (sw_inv_purpose == nullptr) {
                res.jsonValue = redfish::messages::internalError();
                res.jsonValue = messages::internalError();
                return;
              }
              BMCWEB_LOG_DEBUG << "sw_inv_purpose = " << sw_inv_purpose;
              std::size_t last_pos = sw_inv_purpose->rfind(".");
              if (last_pos != std::string::npos) {
                res.jsonValue["SoftwareId"] =
                    sw_inv_purpose->substr(last_pos + 1);
              } else {
                BMCWEB_LOG_ERROR << "Can't parse software purpose!";
              }

              // Version
              it = propertiesList.find("Version");
              if (it != propertiesList.end()) {
                const std::string *version =
                    mapbox::getPtr<const std::string>(it->second);
                if (version == nullptr) {
                  res.jsonValue = redfish::messages::internalError();
                  res.jsonValue = messages::internalError();
                  return;
                }
                res.jsonValue["Version"] =
                    *(mapbox::getPtr<const std::string>(it->second));
              } else {
                BMCWEB_LOG_DEBUG << "Can't find version info!";
              }

              res.end();
            },
            connectionName, obj.first, "org.freedesktop.DBus.Properties",
            "GetAll", "xyz.openbmc_project.Software.Version");
      }
      if (!fw_id_found) {
        res.result(boost::beast::http::status::not_found);
        res.end();
        return;
      }
    });
  }

  OnDemandSoftwareInventoryProvider softwareInventoryProvider;
};

}  // namespace redfish
