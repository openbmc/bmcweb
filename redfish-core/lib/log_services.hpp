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

#include <experimental/filesystem>
#include "node.hpp"
#include <boost/container/flat_map.hpp>

namespace redfish {

constexpr char const *CPU_LOG_OBJECT = "com.intel.CpuDebugLog";
constexpr char const *CPU_LOG_PATH = "/com/intel/CpuDebugLog";
constexpr char const *CPU_LOG_IMMEDIATE_PATH =
    "/com/intel/CpuDebugLog/Immediate";
constexpr char const *CPU_LOG_INTERFACE = "com.intel.CpuDebugLog";
constexpr char const *CPU_LOG_IMMEDIATE_INTERFACE =
    "com.intel.CpuDebugLog.Immediate";
constexpr char const *CPU_LOG_RAW_PECI_INTERFACE =
    "com.intel.CpuDebugLog.SendRawPeci";

namespace fs = std::experimental::filesystem;

class LogServiceCollection : public Node {
 public:
  template <typename CrowApp>
  LogServiceCollection(CrowApp &app)
      : Node(app, "/redfish/v1/Managers/openbmc/LogServices/") {
    // Collections use static ID for SubRoute to add to its parent, but only
    // load dynamic data so the duplicate static members don't get displayed
    Node::json["@odata.id"] = "/redfish/v1/Managers/openbmc/LogServices";
    entityPrivileges = {
        {boost::beast::http::verb::get, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::head, {{"ConfigureComponents"}}},
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
    // Collections don't include the static data added by SubRoute because it
    // has a duplicate entry for members
    res.jsonValue["@odata.type"] = "#LogServiceCollection.LogServiceCollection";
    res.jsonValue["@odata.context"] =
        "/redfish/v1/"
        "$metadata#LogServiceCollection.LogServiceCollection";
    res.jsonValue["@odata.id"] = "/redfish/v1/Managers/openbmc/LogServices";
    res.jsonValue["Name"] = "Open BMC Log Services Collection";
    res.jsonValue["Description"] = "Collection of LogServices for this Manager";
    nlohmann::json &logserviceArray = res.jsonValue["Members"];
    logserviceArray = nlohmann::json::array();
#ifdef BMCWEB_ENABLE_REDFISH_CPU_LOG
    logserviceArray.push_back(
        {{"@odata.id", "/redfish/v1/Managers/openbmc/LogServices/CpuLog"}});
#endif
    res.jsonValue["Members@odata.count"] = logserviceArray.size();
    res.end();
  }
};


class CpuLogService : public Node {
 public:
  template <typename CrowApp>
  CpuLogService(CrowApp &app)
      : Node(app, "/redfish/v1/Managers/openbmc/LogServices/CpuLog") {
    // Set the id for SubRoute
    Node::json["@odata.id"] = "/redfish/v1/Managers/openbmc/LogServices/CpuLog";
    entityPrivileges = {
        {boost::beast::http::verb::get, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::head, {{"ConfigureComponents"}}},
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
    // Copy over the static data to include the entries added by SubRoute
    res.jsonValue = Node::json;
    res.jsonValue["@odata.type"] = "#LogService.v1_1_0.LogService";
    res.jsonValue["@odata.context"] =
        "/redfish/v1/"
        "$metadata#LogService.LogService";
    res.jsonValue["Name"] = "Open BMC CPU Log Service";
    res.jsonValue["Description"] = "CPU Log Service";
    res.jsonValue["Id"] = "CPU Log";
    res.jsonValue["OverWritePolicy"] = "WrapsWhenFull";
    res.jsonValue["MaxNumberOfRecords"] = 3;
    res.jsonValue["Actions"] = {
        {"Oem",
         {{"#CpuLog.Immediate",
           {{"target",
             "/redfish/v1/Managers/openbmc/LogServices/CpuLog/Actions/Oem/"
             "CpuLog.Immediate"}}}}}};

#ifdef BMCWEB_ENABLE_REDFISH_RAW_PECI
    res.jsonValue["Actions"]["Oem"].push_back(
        {"#CpuLog.SendRawPeci",
         {{"target",
           "/redfish/v1/Managers/openbmc/LogServices/CpuLog/Actions/Oem/"
           "CpuLog.SendRawPeci"}}});
#endif
    res.end();
  }
};

class CpuLogEntryCollection : public Node {
 public:
  template <typename CrowApp>
  CpuLogEntryCollection(CrowApp &app)
      : Node(app, "/redfish/v1/Managers/openbmc/LogServices/CpuLog/Entries") {
    // Collections use static ID for SubRoute to add to its parent, but only
    // load dynamic data so the duplicate static members don't get displayed
    Node::json["@odata.id"] =
        "/redfish/v1/Managers/openbmc/LogServices/CpuLog/Entries";
    entityPrivileges = {
        {boost::beast::http::verb::get, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::head, {{"ConfigureComponents"}}},
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
    // Collections don't include the static data added by SubRoute because it
    // has a duplicate entry for members
    auto getLogEntriesCallback = [&res](const boost::system::error_code ec,
                                        const std::vector<std::string> &resp) {
      if (ec) {
        if (ec.value() != boost::system::errc::no_such_file_or_directory) {
          BMCWEB_LOG_DEBUG << "failed to get entries ec: " << ec.message();
          res.result(boost::beast::http::status::internal_server_error);
          res.end();
          return;
        }
      }
      res.jsonValue["@odata.type"] = "#LogEntryCollection.LogEntryCollection";
      res.jsonValue["@odata.context"] =
          "/redfish/v1/"
          "$metadata#LogEntryCollection.LogEntryCollection";
      res.jsonValue["Name"] = "Open BMC CPU Log Entries";
      res.jsonValue["Description"] = "Collection of CPU Log Entries";
      nlohmann::json &logentry_array = res.jsonValue["Members"];
      logentry_array = nlohmann::json::array();
      for (const std::string &objpath : resp) {
        // Don't list the immediate log
        if (objpath.compare(CPU_LOG_IMMEDIATE_PATH) == 0) {
          continue;
        }
        std::size_t last_pos = objpath.rfind("/");
        if (last_pos != std::string::npos) {
          logentry_array.push_back(
              {{"@odata.id",
                "/redfish/v1/Managers/openbmc/LogServices/CpuLog/Entries/" +
                    objpath.substr(last_pos + 1)}});
        }
      }
      res.jsonValue["Members@odata.count"] = logentry_array.size();
      res.end();
    };
    crow::connections::systemBus->async_method_call(
        std::move(getLogEntriesCallback), "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths", "", 0,
        std::array<const char *, 1>{CPU_LOG_INTERFACE});
  }
};

std::string getLogCreatedTime(const nlohmann::json &cpuLog) {
  nlohmann::json::const_iterator metaIt = cpuLog.find("metadata");
  if (metaIt != cpuLog.end()) {
    nlohmann::json::const_iterator tsIt = metaIt->find("timestamp");
    if (tsIt != metaIt->end()) {
      const std::string *logTime = tsIt->get_ptr<const std::string *>();
      if (logTime != nullptr) {
        return *logTime;
      }
    }
  }
  BMCWEB_LOG_DEBUG << "failed to find log timestamp";

  return std::string();
}

class CpuLogEntry : public Node {
 public:
  CpuLogEntry(CrowApp &app)
      : Node(app,
             "/redfish/v1/Managers/openbmc/LogServices/CpuLog/Entries/<str>/",
             std::string()) {
    entityPrivileges = {
        {boost::beast::http::verb::get, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::head, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
  }

 private:
  void doGet(crow::Response &res, const crow::Request &req,
             const std::vector<std::string> &params) override {
    if (params.size() != 1) {
      res.result(boost::beast::http::status::internal_server_error);
      res.end();
      return;
    }
    const uint8_t log_id = std::atoi(params[0].c_str());
    auto getStoredLogCallback =
        [&res, log_id](const boost::system::error_code ec,
                       const sdbusplus::message::variant<std::string> &resp) {
          if (ec) {
            BMCWEB_LOG_DEBUG << "failed to get log ec: " << ec.message();
            res.result(boost::beast::http::status::internal_server_error);
            res.end();
            return;
          }
          const std::string *log = mapbox::getPtr<const std::string>(resp);
          if (log == nullptr) {
            res.result(boost::beast::http::status::internal_server_error);
            res.end();
            return;
          }
          nlohmann::json j = nlohmann::json::parse(*log, nullptr, false);
          if (j.is_discarded()) {
            res.result(boost::beast::http::status::internal_server_error);
            res.end();
            return;
          }
          std::string t = getLogCreatedTime(j);
          res.jsonValue = {
              {"@odata.type", "#LogEntry.v1_3_0.LogEntry"},
              {"@odata.context", "/redfish/v1/$metadata#LogEntry.LogEntry"},
              {"@odata.id",
               "/redfish/v1/Managers/openbmc/LogServices/CpuLog/Entries/" +
                   std::to_string(log_id)},
              {"Name", "CPU Debug Log"},
              {"Id", log_id},
              {"EntryType", "Oem"},
              {"OemRecordFormat", "Intel CPU Log"},
              {"Oem", {{"Intel", std::move(j)}}},
              {"Created", std::move(t)}};
          res.end();
        };
    crow::connections::systemBus->async_method_call(
        std::move(getStoredLogCallback), CPU_LOG_OBJECT,
        CPU_LOG_PATH + std::string("/") + std::to_string(log_id),
        "org.freedesktop.DBus.Properties", "Get", CPU_LOG_INTERFACE, "Log");
  }
};

class ImmediateCpuLog : public Node {
 public:
  ImmediateCpuLog(CrowApp &app)
      : Node(app,
             "/redfish/v1/Managers/openbmc/LogServices/CpuLog/Actions/Oem/"
             "CpuLog.Immediate") {
    entityPrivileges = {
        {boost::beast::http::verb::get, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::head, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
  }

 private:
  void doPost(crow::Response &res, const crow::Request &req,
              const std::vector<std::string> &params) override {
    static std::unique_ptr<sdbusplus::bus::match::match> immediateLogMatcher;

    // Only allow one Immediate Log request at a time
    if (immediateLogMatcher != nullptr) {
      res.addHeader("Retry-After", "30");
      res.result(boost::beast::http::status::service_unavailable);
      messages::addMessageToJson(res.jsonValue,
                                 messages::serviceTemporarilyUnavailable("30"),
                                 "/CpuLog.Immediate");
      res.end();
      return;
    }
    // Make this static so it survives outside this method
    static boost::asio::deadline_timer timeout(*req.ioService);

    timeout.expires_from_now(boost::posix_time::seconds(30));
    timeout.async_wait([&res](const boost::system::error_code &ec) {
      immediateLogMatcher = nullptr;
      if (ec) {
        // operation_aborted is expected if timer is canceled before completion.
        if (ec != boost::asio::error::operation_aborted) {
          BMCWEB_LOG_ERROR << "Async_wait failed " << ec;
        }
        return;
      }
      BMCWEB_LOG_ERROR << "Timed out waiting for immediate log";

      res.result(boost::beast::http::status::internal_server_error);
      res.end();
    });

    auto immediateLogMatcherCallback = [&res](sdbusplus::message::message &m) {
      BMCWEB_LOG_DEBUG << "Immediate log available match fired";
      boost::system::error_code ec;
      timeout.cancel(ec);
      if (ec) {
        BMCWEB_LOG_ERROR << "error canceling timer " << ec;
      }
      sdbusplus::message::object_path obj_path;
      boost::container::flat_map<
          std::string,
          boost::container::flat_map<std::string,
                                     sdbusplus::message::variant<std::string>>>
          interfaces_added;
      m.read(obj_path, interfaces_added);
      const std::string *log = mapbox::getPtr<const std::string>(
          interfaces_added[CPU_LOG_INTERFACE]["Log"]);
      if (log == nullptr) {
        res.result(boost::beast::http::status::internal_server_error);
        res.end();
        // Careful with immediateLogMatcher.  It is a unique_ptr to the match
        // object inside which this lambda is executing.  Once it is set to
        // nullptr, the match object will be destroyed and the lambda will lose
        // its context, including res, so it needs to be the last thing done.
        immediateLogMatcher = nullptr;
        return;
      }
      nlohmann::json j = nlohmann::json::parse(*log, nullptr, false);
      if (j.is_discarded()) {
        res.result(boost::beast::http::status::internal_server_error);
        res.end();
        // Careful with immediateLogMatcher.  It is a unique_ptr to the match
        // object inside which this lambda is executing.  Once it is set to
        // nullptr, the match object will be destroyed and the lambda will lose
        // its context, including res, so it needs to be the last thing done.
        immediateLogMatcher = nullptr;
        return;
      }
      std::string t = getLogCreatedTime(j);
      res.jsonValue = {
          {"@odata.type", "#LogEntry.v1_3_0.LogEntry"},
          {"@odata.context", "/redfish/v1/$metadata#LogEntry.LogEntry"},
          {"Name", "CPU Debug Log"},
          {"EntryType", "Oem"},
          {"OemRecordFormat", "Intel CPU Log"},
          {"Oem", {{"Intel", std::move(j)}}},
          {"Created", std::move(t)}};
      res.end();
      // Careful with immediateLogMatcher.  It is a unique_ptr to the match
      // object inside which this lambda is executing.  Once it is set to
      // nullptr, the match object will be destroyed and the lambda will lose
      // its context, including res, so it needs to be the last thing done.
      immediateLogMatcher = nullptr;
    };
    immediateLogMatcher = std::make_unique<sdbusplus::bus::match::match>(
        *crow::connections::systemBus,
        sdbusplus::bus::match::rules::interfacesAdded() +
            sdbusplus::bus::match::rules::argNpath(0, CPU_LOG_IMMEDIATE_PATH),
        std::move(immediateLogMatcherCallback));

    auto generateImmediateLogCallback =
        [&res](const boost::system::error_code ec, const std::string &resp) {
          if (ec) {
            if (ec.value() == boost::system::errc::operation_not_supported) {
              messages::addMessageToJson(res.jsonValue,
                                         messages::resourceInStandby(),
                                         "/CpuLog.Immediate");
              res.result(boost::beast::http::status::service_unavailable);
            } else {
              res.result(boost::beast::http::status::internal_server_error);
            }
            res.end();
            boost::system::error_code timeoutec;
            timeout.cancel(timeoutec);
            if (timeoutec) {
              BMCWEB_LOG_ERROR << "error canceling timer " << timeoutec;
            }
            immediateLogMatcher = nullptr;
            return;
          }
        };
    crow::connections::systemBus->async_method_call(
        std::move(generateImmediateLogCallback), CPU_LOG_OBJECT, CPU_LOG_PATH,
        CPU_LOG_IMMEDIATE_INTERFACE, "GenerateImmediateLog");
  }
};

class SendRawPeci : public Node {
 public:
  SendRawPeci(CrowApp &app)
      : Node(app,
             "/redfish/v1/Managers/openbmc/LogServices/CpuLog/Actions/Oem/"
             "CpuLog.SendRawPeci") {
    entityPrivileges = {
        {boost::beast::http::verb::get, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::head, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
  }

 private:
  void doPost(crow::Response &res, const crow::Request &req,
              const std::vector<std::string> &params) override {
    // Get the Raw PECI command from the request
    nlohmann::json rawPeciCmd;
    if (!json_util::processJsonFromRequest(res, req, rawPeciCmd)) {
      return;
    }
    // Get the Client Address from the request
    nlohmann::json::const_iterator caIt = rawPeciCmd.find("ClientAddress");
    if (caIt == rawPeciCmd.end()) {
      messages::addMessageToJson(res.jsonValue,
                                 messages::propertyMissing("ClientAddress"),
                                 "/ClientAddress");
      res.result(boost::beast::http::status::bad_request);
      res.end();
      return;
    }
    const uint64_t *ca = caIt->get_ptr<const uint64_t *>();
    if (ca == nullptr) {
      messages::addMessageToJson(
          res.jsonValue,
          messages::propertyValueTypeError(caIt->dump(), "ClientAddress"),
          "/ClientAddress");
      res.result(boost::beast::http::status::bad_request);
      res.end();
      return;
    }
    // Get the Read Length from the request
    const uint8_t clientAddress = static_cast<uint8_t>(*ca);
    nlohmann::json::const_iterator rlIt = rawPeciCmd.find("ReadLength");
    if (rlIt == rawPeciCmd.end()) {
      messages::addMessageToJson(res.jsonValue,
                                 messages::propertyMissing("ReadLength"),
                                 "/ReadLength");
      res.result(boost::beast::http::status::bad_request);
      res.end();
      return;
    }
    const uint64_t *rl = rlIt->get_ptr<const uint64_t *>();
    if (rl == nullptr) {
      messages::addMessageToJson(
          res.jsonValue,
          messages::propertyValueTypeError(rlIt->dump(), "ReadLength"),
          "/ReadLength");
      res.result(boost::beast::http::status::bad_request);
      res.end();
      return;
    }
    // Get the PECI Command from the request
    const uint32_t readLength = static_cast<uint32_t>(*rl);
    nlohmann::json::const_iterator pcIt = rawPeciCmd.find("PECICommand");
    if (pcIt == rawPeciCmd.end()) {
      messages::addMessageToJson(res.jsonValue,
                                 messages::propertyMissing("PECICommand"),
                                 "/PECICommand");
      res.result(boost::beast::http::status::bad_request);
      res.end();
      return;
    }
    std::vector<uint8_t> peciCommand;
    for (auto pc : *pcIt) {
      const uint64_t *val = pc.get_ptr<const uint64_t *>();
      if (val == nullptr) {
        messages::addMessageToJson(
            res.jsonValue,
            messages::propertyValueTypeError(
                pc.dump(), "PECICommand/" + std::to_string(peciCommand.size())),
            "/PECICommand");
        res.result(boost::beast::http::status::bad_request);
        res.end();
        return;
      }
      peciCommand.push_back(static_cast<uint8_t>(*val));
    }
    // Callback to return the Raw PECI response
    auto sendRawPeciCallback = [&res](const boost::system::error_code ec,
                                      const std::vector<uint8_t> &resp) {
      if (ec) {
        BMCWEB_LOG_DEBUG << "failed to send PECI command ec: " << ec.message();
        res.result(boost::beast::http::status::internal_server_error);
        res.end();
        return;
      }
      res.jsonValue = {{"Name", "PECI Command Response"},
                       {"PECIResponse", resp}};
      res.end();
    };
    // Call the SendRawPECI command with the provided data
    crow::connections::systemBus->async_method_call(
        std::move(sendRawPeciCallback), CPU_LOG_OBJECT, CPU_LOG_PATH,
        CPU_LOG_RAW_PECI_INTERFACE, "SendRawPeci", clientAddress, readLength,
        peciCommand);
  }
};

}  // namespace redfish
