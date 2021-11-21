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

#pragma once
#include <tinyxml2.h>

#include <async_resp_class_decl.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/container/flat_set.hpp>
#include <dbus_singleton.hpp>
#include <dbus_utility.hpp>
#include <sdbusplus/message/types.hpp>
#include "../http/http_request_class_decl.hpp"
#include "../http/app_class_decl.hpp"

#include <filesystem>
#include <fstream>
#include <regex>
#include <utility>

namespace crow
{
namespace openbmc_mapper
{

using GetSubTreeType = std::vector<
    std::pair<std::string,
              std::vector<std::pair<std::string, std::vector<std::string>>>>>;

const constexpr char* notFoundMsg = "404 Not Found";
const constexpr char* badReqMsg = "400 Bad Request";
const constexpr char* methodNotAllowedMsg = "405 Method Not Allowed";
const constexpr char* forbiddenMsg = "403 Forbidden";
const constexpr char* methodFailedMsg = "500 Method Call Failed";
const constexpr char* methodOutputFailedMsg = "500 Method Output Error";
const constexpr char* notFoundDesc =
    "org.freedesktop.DBus.Error.FileNotFound: path or object not found";
const constexpr char* propNotFoundDesc =
    "The specified property cannot be found";
const constexpr char* noJsonDesc = "No JSON object could be decoded";
const constexpr char* methodNotFoundDesc =
    "The specified method cannot be found";
const constexpr char* methodNotAllowedDesc = "Method not allowed";
const constexpr char* forbiddenPropDesc =
    "The specified property cannot be created";
const constexpr char* forbiddenResDesc =
    "The specified resource cannot be created";

void setErrorResponse(crow::Response& res,
                             boost::beast::http::status result,
                             const std::string& desc,
                             const std::string_view msg);

void
    introspectObjects(const std::string& processName,
                      const std::string& objectPath,
                      const std::shared_ptr<bmcweb::AsyncResp>& transaction);

void getPropertiesForEnumerate(
    const std::string& objectPath, const std::string& service,
    const std::string& interface,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp);


// Find any results that weren't picked up by ObjectManagers, to be
// called after all ObjectManagers are searched for and called.
void findRemainingObjectsForEnumerate(
    const std::string& objectPath,
    const std::shared_ptr<GetSubTreeType>& subtree,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp);

struct InProgressEnumerateData
{
    InProgressEnumerateData(
        const std::string& objectPathIn,
        const std::shared_ptr<bmcweb::AsyncResp>& asyncRespIn) :
        objectPath(objectPathIn),
        asyncResp(asyncRespIn)
    {}

    ~InProgressEnumerateData();

    const std::string objectPath;
    std::shared_ptr<GetSubTreeType> subtree;
    std::shared_ptr<bmcweb::AsyncResp> asyncResp;
};

void getManagedObjectsForEnumerate(
    const std::string& objectName, const std::string& objectManagerPath,
    const std::string& connectionName,
    const std::shared_ptr<InProgressEnumerateData>& transaction);

void findObjectManagerPathForEnumerate(
    const std::string& objectName, const std::string& connectionName,
    const std::shared_ptr<InProgressEnumerateData>& transaction);

// Uses GetObject to add the object info about the target /enumerate path to
// the results of GetSubTree, as GetSubTree will not return info for the
// target path, and then continues on enumerating the rest of the tree.
void getObjectAndEnumerate(
    const std::shared_ptr<InProgressEnumerateData>& transaction);

// Structure for storing data on an in progress action
struct InProgressActionData
{
    InProgressActionData(crow::Response& resIn) : res(resIn)
    {}
    ~InProgressActionData()
    {
        // Methods could have been called across different owners
        // and interfaces, where some calls failed and some passed.
        //
        // The rules for this are:
        // * if no method was called - error
        // * if a method failed and none passed - error
        //   (converse: if at least one method passed - OK)
        // * for the method output:
        //   * if output processing didn't fail, return the data

        // Only deal with method returns if nothing failed earlier
        if (res.result() == boost::beast::http::status::ok)
        {
            if (!methodPassed)
            {
                if (!methodFailed)
                {
                    setErrorResponse(res, boost::beast::http::status::not_found,
                                     methodNotFoundDesc, notFoundMsg);
                }
            }
            else
            {
                if (outputFailed)
                {
                    setErrorResponse(
                        res, boost::beast::http::status::internal_server_error,
                        "Method output failure", methodOutputFailedMsg);
                }
                else
                {
                    res.jsonValue = {{"status", "ok"},
                                     {"message", "200 OK"},
                                     {"data", methodResponse}};
                }
            }
        }

        res.end();
    }

    void setErrorStatus(const std::string& desc)
    {
        setErrorResponse(res, boost::beast::http::status::bad_request, desc,
                         badReqMsg);
    }
    crow::Response& res;
    std::string path;
    std::string methodName;
    std::string interfaceName;
    bool methodPassed = false;
    bool methodFailed = false;
    bool outputFailed = false;
    bool convertedToArray = false;
    nlohmann::json methodResponse;
    nlohmann::json arguments;
};

inline std::vector<std::string> dbusArgSplit(const std::string& string)
{
    std::vector<std::string> ret;
    if (string.empty())
    {
        return ret;
    }
    ret.emplace_back("");
    int containerDepth = 0;

    for (std::string::const_iterator character = string.begin();
         character != string.end(); character++)
    {
        ret.back() += *character;
        switch (*character)
        {
            case ('a'):
                break;
            case ('('):
            case ('{'):
                containerDepth++;
                break;
            case ('}'):
            case (')'):
                containerDepth--;
                if (containerDepth == 0)
                {
                    if (character + 1 != string.end())
                    {
                        ret.emplace_back("");
                    }
                }
                break;
            default:
                if (containerDepth == 0)
                {
                    if (character + 1 != string.end())
                    {
                        ret.emplace_back("");
                    }
                }
                break;
        }
    }

    return ret;
}

int convertJsonToDbus(sd_bus_message* m, const std::string& argType,
                             const nlohmann::json& inputJson);

template <typename T>
int readMessageItem(const std::string& typeCode, sdbusplus::message::message& m,
                    nlohmann::json& data);

int convertDBusToJSON(const std::string& returnType,
                      sdbusplus::message::message& m, nlohmann::json& response);

int readDictEntryFromMessage(const std::string& typeCode,
                                    sdbusplus::message::message& m,
                                    nlohmann::json& object);

int readArrayFromMessage(const std::string& typeCode,
                                sdbusplus::message::message& m,
                                nlohmann::json& data);

int readStructFromMessage(const std::string& typeCode,
                                 sdbusplus::message::message& m,
                                 nlohmann::json& data);

int readVariantFromMessage(sdbusplus::message::message& m,
                                  nlohmann::json& data);

int convertDBusToJSON(const std::string& returnType,
                             sdbusplus::message::message& m,
                             nlohmann::json& response);

void handleMethodResponse(
    const std::shared_ptr<InProgressActionData>& transaction,
    sdbusplus::message::message& m, const std::string& returnType);

void findActionOnInterface(
    const std::shared_ptr<InProgressActionData>& transaction,
    const std::string& connectionName);

void handleAction(const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& objectPath,
                         const std::string& methodName);

void handleDelete(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& objectPath);


void handleList(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& objectPath, int32_t depth = 0);

void handleEnumerate(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& objectPath);

void handleGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      std::string& objectPath, std::string& destProperty);

struct AsyncPutRequest
{
    AsyncPutRequest(const std::shared_ptr<bmcweb::AsyncResp>& resIn) :
        asyncResp(resIn)
    {}
    ~AsyncPutRequest()
    {
        if (asyncResp->res.jsonValue.empty())
        {
            setErrorResponse(asyncResp->res,
                             boost::beast::http::status::forbidden,
                             forbiddenMsg, forbiddenPropDesc);
        }
    }

    void setErrorStatus(const std::string& desc)
    {
        setErrorResponse(asyncResp->res,
                         boost::beast::http::status::internal_server_error,
                         desc, badReqMsg);
    }

    const std::shared_ptr<bmcweb::AsyncResp> asyncResp;
    std::string objectPath;
    std::string propertyName;
    nlohmann::json propertyValue;
};

void handlePut(const crow::Request& req,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& objectPath,
                      const std::string& destProperty);

void handleDBusUrl(const crow::Request& req,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          std::string& objectPath);

void requestRoutes(App& app);

} // namespace openbmc_mapper
} // namespace crow
