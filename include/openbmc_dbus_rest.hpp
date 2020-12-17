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

#include <app.hpp>
#include <async_resp.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/container/flat_set.hpp>
#include <dbus_singleton.hpp>
#include <dbus_utility.hpp>
#include <sdbusplus/message/types.hpp>

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

inline void setErrorResponse(crow::Response& res,
                             boost::beast::http::status result,
                             const std::string& desc,
                             const std::string_view msg)
{
    res.result(result);
    res.jsonValue = {{"data", {{"description", desc}}},
                     {"message", msg},
                     {"status", "error"}};
}

inline void
    introspectObjects(const std::string& processName,
                      const std::string& objectPath,
                      const std::shared_ptr<bmcweb::AsyncResp>& transaction)
{
    if (transaction->res.jsonValue.is_null())
    {
        transaction->res.jsonValue = {{"status", "ok"},
                                      {"bus_name", processName},
                                      {"objects", nlohmann::json::array()}};
    }

    crow::connections::systemBus->async_method_call(
        [transaction, processName{std::string(processName)},
         objectPath{std::string(objectPath)}](
            const boost::system::error_code ec,
            const std::string& introspectXml) {
            if (ec)
            {
                BMCWEB_LOG_ERROR
                    << "Introspect call failed with error: " << ec.message()
                    << " on process: " << processName << " path: " << objectPath
                    << "\n";
                return;
            }
            transaction->res.jsonValue["objects"].push_back(
                {{"path", objectPath}});

            tinyxml2::XMLDocument doc;

            doc.Parse(introspectXml.c_str());
            tinyxml2::XMLNode* pRoot = doc.FirstChildElement("node");
            if (pRoot == nullptr)
            {
                BMCWEB_LOG_ERROR << "XML document failed to parse "
                                 << processName << " " << objectPath << "\n";
            }
            else
            {
                tinyxml2::XMLElement* node = pRoot->FirstChildElement("node");
                while (node != nullptr)
                {
                    const char* childPath = node->Attribute("name");
                    if (childPath != nullptr)
                    {
                        std::string newpath;
                        if (objectPath != "/")
                        {
                            newpath += objectPath;
                        }
                        newpath += std::string("/") + childPath;
                        // introspect the subobjects as well
                        introspectObjects(processName, newpath, transaction);
                    }

                    node = node->NextSiblingElement("node");
                }
            }
        },
        processName, objectPath, "org.freedesktop.DBus.Introspectable",
        "Introspect");
}

inline void getPropertiesForEnumerate(
    const std::string& objectPath, const std::string& service,
    const std::string& interface,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG << "getPropertiesForEnumerate " << objectPath << " "
                     << service << " " << interface;

    crow::connections::systemBus->async_method_call(
        [asyncResp, objectPath, service, interface](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string, dbus::utility::DbusVariantType>>& propertiesList) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "GetAll on path " << objectPath << " iface "
                                 << interface << " service " << service
                                 << " failed with code " << ec;
                return;
            }

            nlohmann::json& dataJson = asyncResp->res.jsonValue["data"];
            nlohmann::json& objectJson = dataJson[objectPath];
            if (objectJson.is_null())
            {
                objectJson = nlohmann::json::object();
            }

            for (const auto& [name, value] : propertiesList)
            {
                nlohmann::json& propertyJson = objectJson[name];
                std::visit([&propertyJson](auto&& val) { propertyJson = val; },
                           value);
            }
        },
        service, objectPath, "org.freedesktop.DBus.Properties", "GetAll",
        interface);
}

// Find any results that weren't picked up by ObjectManagers, to be
// called after all ObjectManagers are searched for and called.
inline void findRemainingObjectsForEnumerate(
    const std::string& objectPath,
    const std::shared_ptr<GetSubTreeType>& subtree,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG << "findRemainingObjectsForEnumerate";
    const nlohmann::json& dataJson = asyncResp->res.jsonValue["data"];

    for (const auto& [path, interface_map] : *subtree)
    {
        if (path == objectPath)
        {
            // An enumerate does not return the target path's properties
            continue;
        }
        if (dataJson.find(path) == dataJson.end())
        {
            for (const auto& [service, interfaces] : interface_map)
            {
                for (const auto& interface : interfaces)
                {
                    if (!boost::starts_with(interface, "org.freedesktop.DBus"))
                    {
                        getPropertiesForEnumerate(path, service, interface,
                                                  asyncResp);
                    }
                }
            }
        }
    }
}

struct InProgressEnumerateData
{
    InProgressEnumerateData(const std::string& objectPathIn,
                            std::shared_ptr<bmcweb::AsyncResp> asyncRespIn) :
        objectPath(objectPathIn),
        asyncResp(std::move(asyncRespIn))
    {}

    ~InProgressEnumerateData()
    {
        findRemainingObjectsForEnumerate(objectPath, subtree, asyncResp);
    }

    const std::string objectPath;
    std::shared_ptr<GetSubTreeType> subtree;
    std::shared_ptr<bmcweb::AsyncResp> asyncResp;
};

inline void getManagedObjectsForEnumerate(
    const std::string& objectName, const std::string& objectManagerPath,
    const std::string& connectionName,
    const std::shared_ptr<InProgressEnumerateData>& transaction)
{
    BMCWEB_LOG_DEBUG << "getManagedObjectsForEnumerate " << objectName
                     << " object_manager_path " << objectManagerPath
                     << " connection_name " << connectionName;
    crow::connections::systemBus->async_method_call(
        [transaction, objectName,
         connectionName](const boost::system::error_code ec,
                         const dbus::utility::ManagedObjectType& objects) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "GetManagedObjects on path " << objectName
                                 << " on connection " << connectionName
                                 << " failed with code " << ec;
                return;
            }

            nlohmann::json& dataJson =
                transaction->asyncResp->res.jsonValue["data"];

            for (const auto& objectPath : objects)
            {
                if (boost::starts_with(objectPath.first.str, objectName))
                {
                    BMCWEB_LOG_DEBUG << "Reading object "
                                     << objectPath.first.str;
                    nlohmann::json& objectJson = dataJson[objectPath.first.str];
                    if (objectJson.is_null())
                    {
                        objectJson = nlohmann::json::object();
                    }
                    for (const auto& interface : objectPath.second)
                    {
                        for (const auto& property : interface.second)
                        {
                            nlohmann::json& propertyJson =
                                objectJson[property.first];
                            std::visit([&propertyJson](
                                           auto&& val) { propertyJson = val; },
                                       property.second);
                        }
                    }
                }
                for (const auto& interface : objectPath.second)
                {
                    if (interface.first == "org.freedesktop.DBus.ObjectManager")
                    {
                        getManagedObjectsForEnumerate(
                            objectPath.first.str, objectPath.first.str,
                            connectionName, transaction);
                    }
                }
            }
        },
        connectionName, objectManagerPath, "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects");
}

inline void findObjectManagerPathForEnumerate(
    const std::string& objectName, const std::string& connectionName,
    const std::shared_ptr<InProgressEnumerateData>& transaction)
{
    BMCWEB_LOG_DEBUG << "Finding objectmanager for path " << objectName
                     << " on connection:" << connectionName;
    crow::connections::systemBus->async_method_call(
        [transaction, objectName, connectionName](
            const boost::system::error_code ec,
            const boost::container::flat_map<
                std::string, boost::container::flat_map<
                                 std::string, std::vector<std::string>>>&
                objects) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "GetAncestors on path " << objectName
                                 << " failed with code " << ec;
                return;
            }

            for (const auto& pathGroup : objects)
            {
                for (const auto& connectionGroup : pathGroup.second)
                {
                    if (connectionGroup.first == connectionName)
                    {
                        // Found the object manager path for this resource.
                        getManagedObjectsForEnumerate(
                            objectName, pathGroup.first, connectionName,
                            transaction);
                        return;
                    }
                }
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetAncestors", objectName,
        std::array<const char*, 1>{"org.freedesktop.DBus.ObjectManager"});
}

// Uses GetObject to add the object info about the target /enumerate path to
// the results of GetSubTree, as GetSubTree will not return info for the
// target path, and then continues on enumerating the rest of the tree.
inline void getObjectAndEnumerate(
    const std::shared_ptr<InProgressEnumerateData>& transaction)
{
    using GetObjectType =
        std::vector<std::pair<std::string, std::vector<std::string>>>;

    crow::connections::systemBus->async_method_call(
        [transaction](const boost::system::error_code ec,
                      const GetObjectType& objects) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "GetObject for path "
                                 << transaction->objectPath
                                 << " failed with code " << ec;
                return;
            }

            BMCWEB_LOG_DEBUG << "GetObject for " << transaction->objectPath
                             << " has " << objects.size() << " entries";
            if (!objects.empty())
            {
                transaction->subtree->emplace_back(transaction->objectPath,
                                                   objects);
            }

            // Map indicating connection name, and the path where the object
            // manager exists
            boost::container::flat_map<std::string, std::string> connections;

            for (const auto& object : *(transaction->subtree))
            {
                for (const auto& connection : object.second)
                {
                    std::string& objectManagerPath =
                        connections[connection.first];
                    for (const auto& interface : connection.second)
                    {
                        BMCWEB_LOG_DEBUG << connection.first
                                         << " has interface " << interface;
                        if (interface == "org.freedesktop.DBus.ObjectManager")
                        {
                            BMCWEB_LOG_DEBUG << "found object manager path "
                                             << object.first;
                            objectManagerPath = object.first;
                        }
                    }
                }
            }
            BMCWEB_LOG_DEBUG << "Got " << connections.size() << " connections";

            for (const auto& connection : connections)
            {
                // If we already know where the object manager is, we don't
                // need to search for it, we can call directly in to
                // getManagedObjects
                if (!connection.second.empty())
                {
                    getManagedObjectsForEnumerate(
                        transaction->objectPath, connection.second,
                        connection.first, transaction);
                }
                else
                {
                    // otherwise we need to find the object manager path
                    // before we can continue
                    findObjectManagerPathForEnumerate(
                        transaction->objectPath, connection.first, transaction);
                }
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject",
        transaction->objectPath, std::array<const char*, 0>());
}

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

inline int convertJsonToDbus(sd_bus_message* m, const std::string& argType,
                             const nlohmann::json& inputJson)
{
    int r = 0;
    BMCWEB_LOG_DEBUG << "Converting " << inputJson.dump()
                     << " to type: " << argType;
    const std::vector<std::string> argTypes = dbusArgSplit(argType);

    // Assume a single object for now.
    const nlohmann::json* j = &inputJson;
    nlohmann::json::const_iterator jIt = inputJson.begin();

    for (const std::string& argCode : argTypes)
    {
        // If we are decoding multiple objects, grab the pointer to the
        // iterator, and increment it for the next loop
        if (argTypes.size() > 1)
        {
            if (jIt == inputJson.end())
            {
                return -2;
            }
            j = &*jIt;
            jIt++;
        }
        const int64_t* intValue = j->get_ptr<const int64_t*>();
        const std::string* stringValue = j->get_ptr<const std::string*>();
        const double* doubleValue = j->get_ptr<const double*>();
        const bool* b = j->get_ptr<const bool*>();
        int64_t v = 0;
        double d = 0.0;

        // Do some basic type conversions that make sense.  uint can be
        // converted to int.  int and uint can be converted to double
        if (intValue == nullptr)
        {
            const uint64_t* uintValue = j->get_ptr<const uint64_t*>();
            if (uintValue != nullptr)
            {
                v = static_cast<int64_t>(*uintValue);
                intValue = &v;
            }
        }
        if (doubleValue == nullptr)
        {
            const uint64_t* uintValue = j->get_ptr<const uint64_t*>();
            if (uintValue != nullptr)
            {
                d = static_cast<double>(*uintValue);
                doubleValue = &d;
            }
        }
        if (doubleValue == nullptr)
        {
            if (intValue != nullptr)
            {
                d = static_cast<double>(*intValue);
                doubleValue = &d;
            }
        }

        if (argCode == "s")
        {
            if (stringValue == nullptr)
            {
                return -1;
            }
            r = sd_bus_message_append_basic(
                m, argCode[0], static_cast<const void*>(stringValue->data()));
            if (r < 0)
            {
                return r;
            }
        }
        else if (argCode == "i")
        {
            if (intValue == nullptr)
            {
                return -1;
            }
            if ((*intValue < std::numeric_limits<int32_t>::lowest()) ||
                (*intValue > std::numeric_limits<int32_t>::max()))
            {
                return -ERANGE;
            }
            int32_t i = static_cast<int32_t>(*intValue);
            r = sd_bus_message_append_basic(m, argCode[0], &i);
            if (r < 0)
            {
                return r;
            }
        }
        else if (argCode == "b")
        {
            // lots of ways bool could be represented here.  Try them all
            int boolInt = false;
            if (intValue != nullptr)
            {
                if (*intValue == 1)
                {
                    boolInt = true;
                }
                else if (*intValue == 0)
                {
                    boolInt = false;
                }
                else
                {
                    return -ERANGE;
                }
            }
            else if (b != nullptr)
            {
                boolInt = *b ? 1 : 0;
            }
            else if (stringValue != nullptr)
            {
                boolInt = boost::istarts_with(*stringValue, "t") ? 1 : 0;
            }
            else
            {
                return -1;
            }
            r = sd_bus_message_append_basic(m, argCode[0], &boolInt);
            if (r < 0)
            {
                return r;
            }
        }
        else if (argCode == "n")
        {
            if (intValue == nullptr)
            {
                return -1;
            }
            if ((*intValue < std::numeric_limits<int16_t>::lowest()) ||
                (*intValue > std::numeric_limits<int16_t>::max()))
            {
                return -ERANGE;
            }
            int16_t n = static_cast<int16_t>(*intValue);
            r = sd_bus_message_append_basic(m, argCode[0], &n);
            if (r < 0)
            {
                return r;
            }
        }
        else if (argCode == "x")
        {
            if (intValue == nullptr)
            {
                return -1;
            }
            r = sd_bus_message_append_basic(m, argCode[0], intValue);
            if (r < 0)
            {
                return r;
            }
        }
        else if (argCode == "y")
        {
            const uint64_t* uintValue = j->get_ptr<const uint64_t*>();
            if (uintValue == nullptr)
            {
                return -1;
            }
            if (*uintValue > std::numeric_limits<uint8_t>::max())
            {
                return -ERANGE;
            }
            uint8_t y = static_cast<uint8_t>(*uintValue);
            r = sd_bus_message_append_basic(m, argCode[0], &y);
        }
        else if (argCode == "q")
        {
            const uint64_t* uintValue = j->get_ptr<const uint64_t*>();
            if (uintValue == nullptr)
            {
                return -1;
            }
            if (*uintValue > std::numeric_limits<uint16_t>::max())
            {
                return -ERANGE;
            }
            uint16_t q = static_cast<uint16_t>(*uintValue);
            r = sd_bus_message_append_basic(m, argCode[0], &q);
        }
        else if (argCode == "u")
        {
            const uint64_t* uintValue = j->get_ptr<const uint64_t*>();
            if (uintValue == nullptr)
            {
                return -1;
            }
            if (*uintValue > std::numeric_limits<uint32_t>::max())
            {
                return -ERANGE;
            }
            uint32_t u = static_cast<uint32_t>(*uintValue);
            r = sd_bus_message_append_basic(m, argCode[0], &u);
        }
        else if (argCode == "t")
        {
            const uint64_t* uintValue = j->get_ptr<const uint64_t*>();
            if (uintValue == nullptr)
            {
                return -1;
            }
            r = sd_bus_message_append_basic(m, argCode[0], uintValue);
        }
        else if (argCode == "d")
        {
            if (doubleValue == nullptr)
            {
                return -1;
            }
            if ((*doubleValue < std::numeric_limits<double>::lowest()) ||
                (*doubleValue > std::numeric_limits<double>::max()))
            {
                return -ERANGE;
            }
            sd_bus_message_append_basic(m, argCode[0], doubleValue);
        }
        else if (boost::starts_with(argCode, "a"))
        {
            std::string containedType = argCode.substr(1);
            r = sd_bus_message_open_container(m, SD_BUS_TYPE_ARRAY,
                                              containedType.c_str());
            if (r < 0)
            {
                return r;
            }

            for (const auto& it : *j)
            {
                r = convertJsonToDbus(m, containedType, it);
                if (r < 0)
                {
                    return r;
                }
            }
            sd_bus_message_close_container(m);
        }
        else if (boost::starts_with(argCode, "v"))
        {
            std::string containedType = argCode.substr(1);
            BMCWEB_LOG_DEBUG << "variant type: " << argCode
                             << " appending variant of type: " << containedType;
            r = sd_bus_message_open_container(m, SD_BUS_TYPE_VARIANT,
                                              containedType.c_str());
            if (r < 0)
            {
                return r;
            }

            r = convertJsonToDbus(m, containedType, inputJson);
            if (r < 0)
            {
                return r;
            }

            r = sd_bus_message_close_container(m);
            if (r < 0)
            {
                return r;
            }
        }
        else if (boost::starts_with(argCode, "(") &&
                 boost::ends_with(argCode, ")"))
        {
            std::string containedType = argCode.substr(1, argCode.size() - 1);
            r = sd_bus_message_open_container(m, SD_BUS_TYPE_STRUCT,
                                              containedType.c_str());
            if (r < 0)
            {
                return r;
            }

            nlohmann::json::const_iterator it = j->begin();
            for (const std::string& argCode2 : dbusArgSplit(argType))
            {
                if (it == j->end())
                {
                    return -1;
                }
                r = convertJsonToDbus(m, argCode2, *it);
                if (r < 0)
                {
                    return r;
                }
                it++;
            }
            r = sd_bus_message_close_container(m);
        }
        else if (boost::starts_with(argCode, "{") &&
                 boost::ends_with(argCode, "}"))
        {
            std::string containedType = argCode.substr(1, argCode.size() - 1);
            r = sd_bus_message_open_container(m, SD_BUS_TYPE_DICT_ENTRY,
                                              containedType.c_str());
            if (r < 0)
            {
                return r;
            }

            std::vector<std::string> codes = dbusArgSplit(containedType);
            if (codes.size() != 2)
            {
                return -1;
            }
            const std::string& keyType = codes[0];
            const std::string& valueType = codes[1];
            for (const auto& it : j->items())
            {
                r = convertJsonToDbus(m, keyType, it.key());
                if (r < 0)
                {
                    return r;
                }

                r = convertJsonToDbus(m, valueType, it.value());
                if (r < 0)
                {
                    return r;
                }
            }
            r = sd_bus_message_close_container(m);
        }
        else
        {
            return -2;
        }
        if (r < 0)
        {
            return r;
        }

        if (argTypes.size() > 1)
        {
            jIt++;
        }
    }

    return r;
}

template <typename T>
int readMessageItem(const std::string& typeCode, sdbusplus::message::message& m,
                    nlohmann::json& data)
{
    T value;

    int r = sd_bus_message_read_basic(m.get(), typeCode.front(), &value);
    if (r < 0)
    {
        BMCWEB_LOG_ERROR << "sd_bus_message_read_basic on type " << typeCode
                         << " failed!";
        return r;
    }

    data = value;
    return 0;
}

int convertDBusToJSON(const std::string& returnType,
                      sdbusplus::message::message& m, nlohmann::json& response);

inline int readDictEntryFromMessage(const std::string& typeCode,
                                    sdbusplus::message::message& m,
                                    nlohmann::json& object)
{
    std::vector<std::string> types = dbusArgSplit(typeCode);
    if (types.size() != 2)
    {
        BMCWEB_LOG_ERROR << "wrong number contained types in dictionary: "
                         << types.size();
        return -1;
    }

    int r = sd_bus_message_enter_container(m.get(), SD_BUS_TYPE_DICT_ENTRY,
                                           typeCode.c_str());
    if (r < 0)
    {
        BMCWEB_LOG_ERROR << "sd_bus_message_enter_container with rc " << r;
        return r;
    }

    nlohmann::json key;
    r = convertDBusToJSON(types[0], m, key);
    if (r < 0)
    {
        return r;
    }

    const std::string* keyPtr = key.get_ptr<const std::string*>();
    if (keyPtr == nullptr)
    {
        // json doesn't support non-string keys.  If we hit this condition,
        // convert the result to a string so we can proceed
        key = key.dump();
        keyPtr = key.get_ptr<const std::string*>();
        // in theory this can't fail now, but lets be paranoid about it
        // anyway
        if (keyPtr == nullptr)
        {
            return -1;
        }
    }
    nlohmann::json& value = object[*keyPtr];

    r = convertDBusToJSON(types[1], m, value);
    if (r < 0)
    {
        return r;
    }

    r = sd_bus_message_exit_container(m.get());
    if (r < 0)
    {
        BMCWEB_LOG_ERROR << "sd_bus_message_exit_container failed";
        return r;
    }

    return 0;
}

inline int readArrayFromMessage(const std::string& typeCode,
                                sdbusplus::message::message& m,
                                nlohmann::json& data)
{
    if (typeCode.size() < 2)
    {
        BMCWEB_LOG_ERROR << "Type code " << typeCode
                         << " too small for an array";
        return -1;
    }

    std::string containedType = typeCode.substr(1);

    int r = sd_bus_message_enter_container(m.get(), SD_BUS_TYPE_ARRAY,
                                           containedType.c_str());
    if (r < 0)
    {
        BMCWEB_LOG_ERROR << "sd_bus_message_enter_container failed with rc "
                         << r;
        return r;
    }

    bool dict = boost::starts_with(containedType, "{") &&
                boost::ends_with(containedType, "}");

    if (dict)
    {
        // Remove the { }
        containedType = containedType.substr(1, containedType.size() - 2);
        data = nlohmann::json::object();
    }
    else
    {
        data = nlohmann::json::array();
    }

    while (true)
    {
        r = sd_bus_message_at_end(m.get(), false);
        if (r < 0)
        {
            BMCWEB_LOG_ERROR << "sd_bus_message_at_end failed";
            return r;
        }

        if (r > 0)
        {
            break;
        }

        // Dictionaries are only ever seen in an array
        if (dict)
        {
            r = readDictEntryFromMessage(containedType, m, data);
            if (r < 0)
            {
                return r;
            }
        }
        else
        {
            data.push_back(nlohmann::json());

            r = convertDBusToJSON(containedType, m, data.back());
            if (r < 0)
            {
                return r;
            }
        }
    }

    r = sd_bus_message_exit_container(m.get());
    if (r < 0)
    {
        BMCWEB_LOG_ERROR << "sd_bus_message_exit_container failed";
        return r;
    }

    return 0;
}

inline int readStructFromMessage(const std::string& typeCode,
                                 sdbusplus::message::message& m,
                                 nlohmann::json& data)
{
    if (typeCode.size() < 3)
    {
        BMCWEB_LOG_ERROR << "Type code " << typeCode
                         << " too small for a struct";
        return -1;
    }

    std::string containedTypes = typeCode.substr(1, typeCode.size() - 2);
    std::vector<std::string> types = dbusArgSplit(containedTypes);

    int r = sd_bus_message_enter_container(m.get(), SD_BUS_TYPE_STRUCT,
                                           containedTypes.c_str());
    if (r < 0)
    {
        BMCWEB_LOG_ERROR << "sd_bus_message_enter_container failed with rc "
                         << r;
        return r;
    }

    for (const std::string& type : types)
    {
        data.push_back(nlohmann::json());
        r = convertDBusToJSON(type, m, data.back());
        if (r < 0)
        {
            return r;
        }
    }

    r = sd_bus_message_exit_container(m.get());
    if (r < 0)
    {
        BMCWEB_LOG_ERROR << "sd_bus_message_exit_container failed";
        return r;
    }
    return 0;
}

inline int readVariantFromMessage(sdbusplus::message::message& m,
                                  nlohmann::json& data)
{
    const char* containerType;
    int r = sd_bus_message_peek_type(m.get(), nullptr, &containerType);
    if (r < 0)
    {
        BMCWEB_LOG_ERROR << "sd_bus_message_peek_type failed";
        return r;
    }

    r = sd_bus_message_enter_container(m.get(), SD_BUS_TYPE_VARIANT,
                                       containerType);
    if (r < 0)
    {
        BMCWEB_LOG_ERROR << "sd_bus_message_enter_container failed with rc "
                         << r;
        return r;
    }

    r = convertDBusToJSON(containerType, m, data);
    if (r < 0)
    {
        return r;
    }

    r = sd_bus_message_exit_container(m.get());
    if (r < 0)
    {
        BMCWEB_LOG_ERROR << "sd_bus_message_enter_container failed";
        return r;
    }

    return 0;
}

inline int convertDBusToJSON(const std::string& returnType,
                             sdbusplus::message::message& m,
                             nlohmann::json& response)
{
    int r = 0;
    const std::vector<std::string> returnTypes = dbusArgSplit(returnType);

    for (const std::string& typeCode : returnTypes)
    {
        nlohmann::json* thisElement = &response;
        if (returnTypes.size() > 1)
        {
            response.push_back(nlohmann::json{});
            thisElement = &response.back();
        }

        if (typeCode == "s" || typeCode == "g" || typeCode == "o")
        {
            r = readMessageItem<char*>(typeCode, m, *thisElement);
            if (r < 0)
            {
                return r;
            }
        }
        else if (typeCode == "b")
        {
            r = readMessageItem<int>(typeCode, m, *thisElement);
            if (r < 0)
            {
                return r;
            }

            *thisElement = static_cast<bool>(thisElement->get<int>());
        }
        else if (typeCode == "u")
        {
            r = readMessageItem<uint32_t>(typeCode, m, *thisElement);
            if (r < 0)
            {
                return r;
            }
        }
        else if (typeCode == "i")
        {
            r = readMessageItem<int32_t>(typeCode, m, *thisElement);
            if (r < 0)
            {
                return r;
            }
        }
        else if (typeCode == "x")
        {
            r = readMessageItem<int64_t>(typeCode, m, *thisElement);
            if (r < 0)
            {
                return r;
            }
        }
        else if (typeCode == "t")
        {
            r = readMessageItem<uint64_t>(typeCode, m, *thisElement);
            if (r < 0)
            {
                return r;
            }
        }
        else if (typeCode == "n")
        {
            r = readMessageItem<int16_t>(typeCode, m, *thisElement);
            if (r < 0)
            {
                return r;
            }
        }
        else if (typeCode == "q")
        {
            r = readMessageItem<uint16_t>(typeCode, m, *thisElement);
            if (r < 0)
            {
                return r;
            }
        }
        else if (typeCode == "y")
        {
            r = readMessageItem<uint8_t>(typeCode, m, *thisElement);
            if (r < 0)
            {
                return r;
            }
        }
        else if (typeCode == "d")
        {
            r = readMessageItem<double>(typeCode, m, *thisElement);
            if (r < 0)
            {
                return r;
            }
        }
        else if (typeCode == "h")
        {
            r = readMessageItem<int>(typeCode, m, *thisElement);
            if (r < 0)
            {
                return r;
            }
        }
        else if (boost::starts_with(typeCode, "a"))
        {
            r = readArrayFromMessage(typeCode, m, *thisElement);
            if (r < 0)
            {
                return r;
            }
        }
        else if (boost::starts_with(typeCode, "(") &&
                 boost::ends_with(typeCode, ")"))
        {
            r = readStructFromMessage(typeCode, m, *thisElement);
            if (r < 0)
            {
                return r;
            }
        }
        else if (boost::starts_with(typeCode, "v"))
        {
            r = readVariantFromMessage(m, *thisElement);
            if (r < 0)
            {
                return r;
            }
        }
        else
        {
            BMCWEB_LOG_ERROR << "Invalid D-Bus signature type " << typeCode;
            return -2;
        }
    }

    return 0;
}

inline void handleMethodResponse(
    const std::shared_ptr<InProgressActionData>& transaction,
    sdbusplus::message::message& m, const std::string& returnType)
{
    nlohmann::json data;

    int r = convertDBusToJSON(returnType, m, data);
    if (r < 0)
    {
        transaction->outputFailed = true;
        return;
    }

    if (data.is_null())
    {
        return;
    }

    if (transaction->methodResponse.is_null())
    {
        transaction->methodResponse = std::move(data);
        return;
    }

    // If they're both dictionaries or arrays, merge into one.
    // Otherwise, make the results an array with every result
    // an entry.  Could also just fail in that case, but it
    // seems better to get the data back somehow.

    if (transaction->methodResponse.is_object() && data.is_object())
    {
        for (const auto& obj : data.items())
        {
            // Note: Will overwrite the data for a duplicate key
            transaction->methodResponse.emplace(obj.key(),
                                                std::move(obj.value()));
        }
        return;
    }

    if (transaction->methodResponse.is_array() && data.is_array())
    {
        for (auto& obj : data)
        {
            transaction->methodResponse.push_back(std::move(obj));
        }
        return;
    }

    if (!transaction->convertedToArray)
    {
        // They are different types. May as well turn them into an array
        nlohmann::json j = std::move(transaction->methodResponse);
        transaction->methodResponse = nlohmann::json::array();
        transaction->methodResponse.push_back(std::move(j));
        transaction->methodResponse.push_back(std::move(data));
        transaction->convertedToArray = true;
    }
    else
    {
        transaction->methodResponse.push_back(std::move(data));
    }
}

inline void findActionOnInterface(
    const std::shared_ptr<InProgressActionData>& transaction,
    const std::string& connectionName)
{
    BMCWEB_LOG_DEBUG << "findActionOnInterface for connection "
                     << connectionName;
    crow::connections::systemBus->async_method_call(
        [transaction, connectionName{std::string(connectionName)}](
            const boost::system::error_code ec,
            const std::string& introspectXml) {
            BMCWEB_LOG_DEBUG << "got xml:\n " << introspectXml;
            if (ec)
            {
                BMCWEB_LOG_ERROR
                    << "Introspect call failed with error: " << ec.message()
                    << " on process: " << connectionName << "\n";
                return;
            }
            tinyxml2::XMLDocument doc;

            doc.Parse(introspectXml.data(), introspectXml.size());
            tinyxml2::XMLNode* pRoot = doc.FirstChildElement("node");
            if (pRoot == nullptr)
            {
                BMCWEB_LOG_ERROR << "XML document failed to parse "
                                 << connectionName << "\n";
                return;
            }
            tinyxml2::XMLElement* interfaceNode =
                pRoot->FirstChildElement("interface");
            while (interfaceNode != nullptr)
            {
                const char* thisInterfaceName =
                    interfaceNode->Attribute("name");
                if (thisInterfaceName != nullptr)
                {
                    if (!transaction->interfaceName.empty() &&
                        (transaction->interfaceName != thisInterfaceName))
                    {
                        interfaceNode =
                            interfaceNode->NextSiblingElement("interface");
                        continue;
                    }

                    tinyxml2::XMLElement* methodNode =
                        interfaceNode->FirstChildElement("method");
                    while (methodNode != nullptr)
                    {
                        const char* thisMethodName =
                            methodNode->Attribute("name");
                        BMCWEB_LOG_DEBUG << "Found method: " << thisMethodName;
                        if (thisMethodName != nullptr &&
                            thisMethodName == transaction->methodName)
                        {
                            BMCWEB_LOG_DEBUG
                                << "Found method named " << thisMethodName
                                << " on interface " << thisInterfaceName;
                            sdbusplus::message::message m =
                                crow::connections::systemBus->new_method_call(
                                    connectionName.c_str(),
                                    transaction->path.c_str(),
                                    thisInterfaceName,
                                    transaction->methodName.c_str());

                            tinyxml2::XMLElement* argumentNode =
                                methodNode->FirstChildElement("arg");

                            std::string returnType;

                            // Find the output type
                            while (argumentNode != nullptr)
                            {
                                const char* argDirection =
                                    argumentNode->Attribute("direction");
                                const char* argType =
                                    argumentNode->Attribute("type");
                                if (argDirection != nullptr &&
                                    argType != nullptr &&
                                    std::string(argDirection) == "out")
                                {
                                    returnType = argType;
                                    break;
                                }
                                argumentNode =
                                    argumentNode->NextSiblingElement("arg");
                            }

                            nlohmann::json::const_iterator argIt =
                                transaction->arguments.begin();

                            argumentNode = methodNode->FirstChildElement("arg");

                            while (argumentNode != nullptr)
                            {
                                const char* argDirection =
                                    argumentNode->Attribute("direction");
                                const char* argType =
                                    argumentNode->Attribute("type");
                                if (argDirection != nullptr &&
                                    argType != nullptr &&
                                    std::string(argDirection) == "in")
                                {
                                    if (argIt == transaction->arguments.end())
                                    {
                                        transaction->setErrorStatus(
                                            "Invalid method args");
                                        return;
                                    }
                                    if (convertJsonToDbus(m.get(),
                                                          std::string(argType),
                                                          *argIt) < 0)
                                    {
                                        transaction->setErrorStatus(
                                            "Invalid method arg type");
                                        return;
                                    }

                                    argIt++;
                                }
                                argumentNode =
                                    argumentNode->NextSiblingElement("arg");
                            }

                            crow::connections::systemBus->async_send(
                                m, [transaction, returnType](
                                       boost::system::error_code ec2,
                                       sdbusplus::message::message& m2) {
                                    if (ec2)
                                    {
                                        transaction->methodFailed = true;
                                        const sd_bus_error* e = m2.get_error();

                                        if (e)
                                        {
                                            setErrorResponse(
                                                transaction->res,
                                                boost::beast::http::status::
                                                    bad_request,
                                                e->name, e->message);
                                        }
                                        else
                                        {
                                            setErrorResponse(
                                                transaction->res,
                                                boost::beast::http::status::
                                                    bad_request,
                                                "Method call failed",
                                                methodFailedMsg);
                                        }
                                        return;
                                    }
                                    transaction->methodPassed = true;

                                    handleMethodResponse(transaction, m2,
                                                         returnType);
                                });
                            break;
                        }
                        methodNode = methodNode->NextSiblingElement("method");
                    }
                }
                interfaceNode = interfaceNode->NextSiblingElement("interface");
            }
        },
        connectionName, transaction->path,
        "org.freedesktop.DBus.Introspectable", "Introspect");
}

inline void handleAction(const crow::Request& req, crow::Response& res,
                         const std::string& objectPath,
                         const std::string& methodName)
{
    BMCWEB_LOG_DEBUG << "handleAction on path: " << objectPath << " and method "
                     << methodName;
    nlohmann::json requestDbusData =
        nlohmann::json::parse(req.body, nullptr, false);

    if (requestDbusData.is_discarded())
    {
        setErrorResponse(res, boost::beast::http::status::bad_request,
                         noJsonDesc, badReqMsg);
        res.end();
        return;
    }
    nlohmann::json::iterator data = requestDbusData.find("data");
    if (data == requestDbusData.end())
    {
        setErrorResponse(res, boost::beast::http::status::bad_request,
                         noJsonDesc, badReqMsg);
        res.end();
        return;
    }

    if (!data->is_array())
    {
        setErrorResponse(res, boost::beast::http::status::bad_request,
                         noJsonDesc, badReqMsg);
        res.end();
        return;
    }
    auto transaction = std::make_shared<InProgressActionData>(res);

    transaction->path = objectPath;
    transaction->methodName = methodName;
    transaction->arguments = std::move(*data);
    crow::connections::systemBus->async_method_call(
        [transaction](
            const boost::system::error_code ec,
            const std::vector<std::pair<std::string, std::vector<std::string>>>&
                interfaceNames) {
            if (ec || interfaceNames.size() <= 0)
            {
                BMCWEB_LOG_ERROR << "Can't find object";
                setErrorResponse(transaction->res,
                                 boost::beast::http::status::not_found,
                                 notFoundDesc, notFoundMsg);
                return;
            }

            BMCWEB_LOG_DEBUG << "GetObject returned " << interfaceNames.size()
                             << " object(s)";

            for (const std::pair<std::string, std::vector<std::string>>&
                     object : interfaceNames)
            {
                findActionOnInterface(transaction, object.first);
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", objectPath,
        std::array<std::string, 0>());
}

inline void handleDelete(crow::Response& res, const std::string& objectPath)
{
    BMCWEB_LOG_DEBUG << "handleDelete on path: " << objectPath;

    crow::connections::systemBus->async_method_call(
        [&res, objectPath](
            const boost::system::error_code ec,
            const std::vector<std::pair<std::string, std::vector<std::string>>>&
                interfaceNames) {
            if (ec || interfaceNames.size() <= 0)
            {
                BMCWEB_LOG_ERROR << "Can't find object";
                setErrorResponse(res,
                                 boost::beast::http::status::method_not_allowed,
                                 methodNotAllowedDesc, methodNotAllowedMsg);
                res.end();
                return;
            }

            auto transaction = std::make_shared<InProgressActionData>(res);
            transaction->path = objectPath;
            transaction->methodName = "Delete";
            transaction->interfaceName = "xyz.openbmc_project.Object.Delete";

            for (const std::pair<std::string, std::vector<std::string>>&
                     object : interfaceNames)
            {
                findActionOnInterface(transaction, object.first);
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", objectPath,
        std::array<const char*, 0>());
}

inline void handleList(crow::Response& res, const std::string& objectPath,
                       int32_t depth = 0)
{
    crow::connections::systemBus->async_method_call(
        [&res](const boost::system::error_code ec,
               std::vector<std::string>& objectPaths) {
            if (ec)
            {
                setErrorResponse(res, boost::beast::http::status::not_found,
                                 notFoundDesc, notFoundMsg);
            }
            else
            {
                res.jsonValue = {{"status", "ok"},
                                 {"message", "200 OK"},
                                 {"data", std::move(objectPaths)}};
            }
            res.end();
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths", objectPath,
        depth, std::array<std::string, 0>());
}

inline void handleEnumerate(crow::Response& res, const std::string& objectPath)
{
    BMCWEB_LOG_DEBUG << "Doing enumerate on " << objectPath;
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>(res);

    asyncResp->res.jsonValue = {{"message", "200 OK"},
                                {"status", "ok"},
                                {"data", nlohmann::json::object()}};

    crow::connections::systemBus->async_method_call(
        [objectPath, asyncResp](const boost::system::error_code ec,
                                GetSubTreeType& objectNames) {
            auto transaction = std::make_shared<InProgressEnumerateData>(
                objectPath, asyncResp);

            transaction->subtree =
                std::make_shared<GetSubTreeType>(std::move(objectNames));

            if (ec)
            {
                BMCWEB_LOG_ERROR << "GetSubTree failed on "
                                 << transaction->objectPath;
                setErrorResponse(transaction->asyncResp->res,
                                 boost::beast::http::status::not_found,
                                 notFoundDesc, notFoundMsg);
                return;
            }

            // Add the data for the path passed in to the results
            // as if GetSubTree returned it, and continue on enumerating
            getObjectAndEnumerate(transaction);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", objectPath, 0,
        std::array<const char*, 0>());
}

inline void handleGet(crow::Response& res, std::string& objectPath,
                      std::string& destProperty)
{
    BMCWEB_LOG_DEBUG << "handleGet: " << objectPath << " prop:" << destProperty;
    std::shared_ptr<std::string> propertyName =
        std::make_shared<std::string>(std::move(destProperty));

    std::shared_ptr<std::string> path =
        std::make_shared<std::string>(std::move(objectPath));

    using GetObjectType =
        std::vector<std::pair<std::string, std::vector<std::string>>>;
    crow::connections::systemBus->async_method_call(
        [&res, path, propertyName](const boost::system::error_code ec,
                                   const GetObjectType& objectNames) {
            if (ec || objectNames.size() <= 0)
            {
                setErrorResponse(res, boost::beast::http::status::not_found,
                                 notFoundDesc, notFoundMsg);
                res.end();
                return;
            }
            std::shared_ptr<nlohmann::json> response =
                std::make_shared<nlohmann::json>(nlohmann::json::object());
            // The mapper should never give us an empty interface names
            // list, but check anyway
            for (const std::pair<std::string, std::vector<std::string>>&
                     connection : objectNames)
            {
                const std::vector<std::string>& interfaceNames =
                    connection.second;

                if (interfaceNames.size() <= 0)
                {
                    setErrorResponse(res, boost::beast::http::status::not_found,
                                     notFoundDesc, notFoundMsg);
                    res.end();
                    return;
                }

                for (const std::string& interface : interfaceNames)
                {
                    sdbusplus::message::message m =
                        crow::connections::systemBus->new_method_call(
                            connection.first.c_str(), path->c_str(),
                            "org.freedesktop.DBus.Properties", "GetAll");
                    m.append(interface);
                    crow::connections::systemBus->async_send(
                        m, [&res, response,
                            propertyName](const boost::system::error_code ec2,
                                          sdbusplus::message::message& msg) {
                            if (ec2)
                            {
                                BMCWEB_LOG_ERROR << "Bad dbus request error: "
                                                 << ec2;
                            }
                            else
                            {
                                nlohmann::json properties;
                                int r =
                                    convertDBusToJSON("a{sv}", msg, properties);
                                if (r < 0)
                                {
                                    BMCWEB_LOG_ERROR
                                        << "convertDBusToJSON failed";
                                }
                                else
                                {
                                    for (auto& prop : properties.items())
                                    {
                                        // if property name is empty, or
                                        // matches our search query, add it
                                        // to the response json

                                        if (propertyName->empty())
                                        {
                                            (*response)[prop.key()] =
                                                std::move(prop.value());
                                        }
                                        else if (prop.key() == *propertyName)
                                        {
                                            *response = std::move(prop.value());
                                        }
                                    }
                                }
                            }
                            if (response.use_count() == 1)
                            {
                                if (!propertyName->empty() && response->empty())
                                {
                                    setErrorResponse(
                                        res,
                                        boost::beast::http::status::not_found,
                                        propNotFoundDesc, notFoundMsg);
                                }
                                else
                                {
                                    res.jsonValue = {{"status", "ok"},
                                                     {"message", "200 OK"},
                                                     {"data", *response}};
                                }
                                res.end();
                            }
                        });
                }
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", *path,
        std::array<std::string, 0>());
}

struct AsyncPutRequest
{
    AsyncPutRequest(crow::Response& resIn) : res(resIn)
    {}
    ~AsyncPutRequest()
    {
        if (res.jsonValue.empty())
        {
            setErrorResponse(res, boost::beast::http::status::forbidden,
                             forbiddenMsg, forbiddenPropDesc);
        }

        res.end();
    }

    void setErrorStatus(const std::string& desc)
    {
        setErrorResponse(res, boost::beast::http::status::internal_server_error,
                         desc, badReqMsg);
    }

    crow::Response& res;
    std::string objectPath;
    std::string propertyName;
    nlohmann::json propertyValue;
};

inline void handlePut(const crow::Request& req, crow::Response& res,
                      const std::string& objectPath,
                      const std::string& destProperty)
{
    if (destProperty.empty())
    {
        setErrorResponse(res, boost::beast::http::status::forbidden,
                         forbiddenResDesc, forbiddenMsg);
        res.end();
        return;
    }

    nlohmann::json requestDbusData =
        nlohmann::json::parse(req.body, nullptr, false);

    if (requestDbusData.is_discarded())
    {
        setErrorResponse(res, boost::beast::http::status::bad_request,
                         noJsonDesc, badReqMsg);
        res.end();
        return;
    }

    nlohmann::json::const_iterator propertyIt = requestDbusData.find("data");
    if (propertyIt == requestDbusData.end())
    {
        setErrorResponse(res, boost::beast::http::status::bad_request,
                         noJsonDesc, badReqMsg);
        res.end();
        return;
    }
    const nlohmann::json& propertySetValue = *propertyIt;
    auto transaction = std::make_shared<AsyncPutRequest>(res);
    transaction->objectPath = objectPath;
    transaction->propertyName = destProperty;
    transaction->propertyValue = propertySetValue;

    using GetObjectType =
        std::vector<std::pair<std::string, std::vector<std::string>>>;

    crow::connections::systemBus->async_method_call(
        [transaction](const boost::system::error_code ec2,
                      const GetObjectType& objectNames) {
            if (!ec2 && objectNames.size() <= 0)
            {
                setErrorResponse(transaction->res,
                                 boost::beast::http::status::not_found,
                                 propNotFoundDesc, notFoundMsg);
                return;
            }

            for (const std::pair<std::string, std::vector<std::string>>&
                     connection : objectNames)
            {
                const std::string& connectionName = connection.first;

                crow::connections::systemBus->async_method_call(
                    [connectionName{std::string(connectionName)},
                     transaction](const boost::system::error_code ec3,
                                  const std::string& introspectXml) {
                        if (ec3)
                        {
                            BMCWEB_LOG_ERROR
                                << "Introspect call failed with error: "
                                << ec3.message()
                                << " on process: " << connectionName;
                            transaction->setErrorStatus("Unexpected Error");
                            return;
                        }
                        tinyxml2::XMLDocument doc;

                        doc.Parse(introspectXml.c_str());
                        tinyxml2::XMLNode* pRoot =
                            doc.FirstChildElement("node");
                        if (pRoot == nullptr)
                        {
                            BMCWEB_LOG_ERROR << "XML document failed to parse: "
                                             << introspectXml;
                            transaction->setErrorStatus("Unexpected Error");
                            return;
                        }
                        tinyxml2::XMLElement* ifaceNode =
                            pRoot->FirstChildElement("interface");
                        while (ifaceNode != nullptr)
                        {
                            const char* interfaceName =
                                ifaceNode->Attribute("name");
                            BMCWEB_LOG_DEBUG << "found interface "
                                             << interfaceName;
                            tinyxml2::XMLElement* propNode =
                                ifaceNode->FirstChildElement("property");
                            while (propNode != nullptr)
                            {
                                const char* propertyName =
                                    propNode->Attribute("name");
                                BMCWEB_LOG_DEBUG << "Found property "
                                                 << propertyName;
                                if (propertyName == transaction->propertyName)
                                {
                                    const char* argType =
                                        propNode->Attribute("type");
                                    if (argType != nullptr)
                                    {
                                        sdbusplus::message::message m =
                                            crow::connections::systemBus
                                                ->new_method_call(
                                                    connectionName.c_str(),
                                                    transaction->objectPath
                                                        .c_str(),
                                                    "org.freedesktop.DBus."
                                                    "Properties",
                                                    "Set");
                                        m.append(interfaceName,
                                                 transaction->propertyName);
                                        int r = sd_bus_message_open_container(
                                            m.get(), SD_BUS_TYPE_VARIANT,
                                            argType);
                                        if (r < 0)
                                        {
                                            transaction->setErrorStatus(
                                                "Unexpected Error");
                                            return;
                                        }
                                        r = convertJsonToDbus(
                                            m.get(), argType,
                                            transaction->propertyValue);
                                        if (r < 0)
                                        {
                                            if (r == -ERANGE)
                                            {
                                                transaction->setErrorStatus(
                                                    "Provided property value "
                                                    "is out of range for the "
                                                    "property type");
                                            }
                                            else
                                            {
                                                transaction->setErrorStatus(
                                                    "Invalid arg type");
                                            }
                                            return;
                                        }
                                        r = sd_bus_message_close_container(
                                            m.get());
                                        if (r < 0)
                                        {
                                            transaction->setErrorStatus(
                                                "Unexpected Error");
                                            return;
                                        }
                                        crow::connections::systemBus
                                            ->async_send(
                                                m,
                                                [transaction](
                                                    boost::system::error_code
                                                        ec,
                                                    sdbusplus::message::message&
                                                        m2) {
                                                    BMCWEB_LOG_DEBUG << "sent";
                                                    if (ec)
                                                    {
                                                        const sd_bus_error* e =
                                                            m2.get_error();
                                                        setErrorResponse(
                                                            transaction->res,
                                                            boost::beast::http::
                                                                status::
                                                                    forbidden,
                                                            (e) ? e->name
                                                                : ec.category()
                                                                      .name(),
                                                            (e) ? e->message
                                                                : ec.message());
                                                    }
                                                    else
                                                    {
                                                        transaction->res
                                                            .jsonValue = {
                                                            {"status", "ok"},
                                                            {"message",
                                                             "200 OK"},
                                                            {"data", nullptr}};
                                                    }
                                                });
                                    }
                                }
                                propNode =
                                    propNode->NextSiblingElement("property");
                            }
                            ifaceNode =
                                ifaceNode->NextSiblingElement("interface");
                        }
                    },
                    connectionName, transaction->objectPath,
                    "org.freedesktop.DBus.Introspectable", "Introspect");
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject",
        transaction->objectPath, std::array<std::string, 0>());
}

inline void handleDBusUrl(const crow::Request& req, crow::Response& res,
                          std::string& objectPath)
{

    // If accessing a single attribute, fill in and update objectPath,
    // otherwise leave destProperty blank
    std::string destProperty = "";
    const char* attrSeperator = "/attr/";
    size_t attrPosition = objectPath.find(attrSeperator);
    if (attrPosition != objectPath.npos)
    {
        destProperty = objectPath.substr(attrPosition + strlen(attrSeperator),
                                         objectPath.length());
        objectPath = objectPath.substr(0, attrPosition);
    }

    if (req.method() == boost::beast::http::verb::post)
    {
        constexpr const char* actionSeperator = "/action/";
        size_t actionPosition = objectPath.find(actionSeperator);
        if (actionPosition != objectPath.npos)
        {
            std::string postProperty =
                objectPath.substr((actionPosition + strlen(actionSeperator)),
                                  objectPath.length());
            objectPath = objectPath.substr(0, actionPosition);
            handleAction(req, res, objectPath, postProperty);
            return;
        }
    }
    else if (req.method() == boost::beast::http::verb::get)
    {
        if (boost::ends_with(objectPath, "/enumerate"))
        {
            objectPath.erase(objectPath.end() - sizeof("enumerate"),
                             objectPath.end());
            handleEnumerate(res, objectPath);
        }
        else if (boost::ends_with(objectPath, "/list"))
        {
            objectPath.erase(objectPath.end() - sizeof("list"),
                             objectPath.end());
            handleList(res, objectPath);
        }
        else
        {
            // Trim any trailing "/" at the end
            if (boost::ends_with(objectPath, "/"))
            {
                objectPath.pop_back();
                handleList(res, objectPath, 1);
            }
            else
            {
                handleGet(res, objectPath, destProperty);
            }
        }
        return;
    }
    else if (req.method() == boost::beast::http::verb::put)
    {
        handlePut(req, res, objectPath, destProperty);
        return;
    }
    else if (req.method() == boost::beast::http::verb::delete_)
    {
        handleDelete(res, objectPath);
        return;
    }

    setErrorResponse(res, boost::beast::http::status::method_not_allowed,
                     methodNotAllowedDesc, methodNotAllowedMsg);
    res.end();
}

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/bus/")
        .privileges({"Login"})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&, crow::Response& res) {
                res.jsonValue = {{"buses", {{{"name", "system"}}}},
                                 {"status", "ok"}};
                res.end();
            });

    BMCWEB_ROUTE(app, "/bus/system/")
        .privileges({"Login"})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&, crow::Response& res) {
                auto myCallback = [&res](const boost::system::error_code ec,
                                         std::vector<std::string>& names) {
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR << "Dbus call failed with code " << ec;
                        res.result(
                            boost::beast::http::status::internal_server_error);
                    }
                    else
                    {
                        std::sort(names.begin(), names.end());
                        res.jsonValue = {{"status", "ok"}};
                        auto& objectsSub = res.jsonValue["objects"];
                        for (auto& name : names)
                        {
                            objectsSub.push_back({{"name", name}});
                        }
                    }
                    res.end();
                };
                crow::connections::systemBus->async_method_call(
                    std::move(myCallback), "org.freedesktop.DBus", "/",
                    "org.freedesktop.DBus", "ListNames");
            });

    BMCWEB_ROUTE(app, "/list/")
        .privileges({"Login"})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&, crow::Response& res) {
                handleList(res, "/");
            });

    BMCWEB_ROUTE(app, "/xyz/<path>")
        .privileges({"Login"})
        .methods(boost::beast::http::verb::get)([](const crow::Request& req,
                                                   crow::Response& res,
                                                   const std::string& path) {
            std::string objectPath = "/xyz/" + path;
            handleDBusUrl(req, res, objectPath);
        });

    BMCWEB_ROUTE(app, "/xyz/<path>")
        .privileges({"ConfigureComponents", "ConfigureManager"})
        .methods(boost::beast::http::verb::put, boost::beast::http::verb::post,
                 boost::beast::http::verb::delete_)(
            [](const crow::Request& req, crow::Response& res,
               const std::string& path) {
                std::string objectPath = "/xyz/" + path;
                handleDBusUrl(req, res, objectPath);
            });

    BMCWEB_ROUTE(app, "/org/<path>")
        .privileges({"Login"})
        .methods(boost::beast::http::verb::get)([](const crow::Request& req,
                                                   crow::Response& res,
                                                   const std::string& path) {
            std::string objectPath = "/org/" + path;
            handleDBusUrl(req, res, objectPath);
        });

    BMCWEB_ROUTE(app, "/org/<path>")
        .privileges({"ConfigureComponents", "ConfigureManager"})
        .methods(boost::beast::http::verb::put, boost::beast::http::verb::post,
                 boost::beast::http::verb::delete_)(
            [](const crow::Request& req, crow::Response& res,
               const std::string& path) {
                std::string objectPath = "/org/" + path;
                handleDBusUrl(req, res, objectPath);
            });

    BMCWEB_ROUTE(app, "/download/dump/<str>/")
        .privileges({"ConfigureManager"})
        .methods(boost::beast::http::verb::get)([](const crow::Request&,
                                                   crow::Response& res,
                                                   const std::string& dumpId) {
            std::regex validFilename(R"(^[\w\- ]+(\.?[\w\- ]*)$)");
            if (!std::regex_match(dumpId, validFilename))
            {
                res.result(boost::beast::http::status::bad_request);
                res.end();
                return;
            }
            std::filesystem::path loc(
                "/var/lib/phosphor-debug-collector/dumps");

            loc /= dumpId;

            if (!std::filesystem::exists(loc) ||
                !std::filesystem::is_directory(loc))
            {
                BMCWEB_LOG_ERROR << loc << "Not found";
                res.result(boost::beast::http::status::not_found);
                res.end();
                return;
            }
            std::filesystem::directory_iterator files(loc);

            for (auto& file : files)
            {
                std::ifstream readFile(file.path());
                if (!readFile.good())
                {
                    continue;
                }

                res.addHeader("Content-Type", "application/octet-stream");

                // Assuming only one dump file will be present in the dump id
                // directory
                std::string dumpFileName = file.path().filename().string();

                // Filename should be in alphanumeric, dot and underscore
                // Its based on phosphor-debug-collector application dumpfile
                // format
                std::regex dumpFileRegex("[a-zA-Z0-9\\._]+");
                if (!std::regex_match(dumpFileName, dumpFileRegex))
                {
                    BMCWEB_LOG_ERROR << "Invalid dump filename "
                                     << dumpFileName;
                    res.result(boost::beast::http::status::not_found);
                    res.end();
                    return;
                }
                std::string contentDispositionParam =
                    "attachment; filename=\"" + dumpFileName + "\"";

                res.addHeader("Content-Disposition", contentDispositionParam);

                res.body() = {std::istreambuf_iterator<char>(readFile),
                              std::istreambuf_iterator<char>()};
                res.end();
                return;
            }
            res.result(boost::beast::http::status::not_found);
            res.end();
            return;
        });

    BMCWEB_ROUTE(app, "/bus/system/<str>/")
        .privileges({"Login"})

        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&, crow::Response& res,
               const std::string& connection) {
                introspectObjects(connection, "/",
                                  std::make_shared<bmcweb::AsyncResp>(res));
            });

    BMCWEB_ROUTE(app, "/bus/system/<str>/<path>")
        .privileges({"ConfigureComponents", "ConfigureManager"})
        .methods(
            boost::beast::http::verb::get,
            boost::beast::http::verb::post)([](const crow::Request& req,
                                               crow::Response& res,
                                               const std::string& processName,
                                               const std::string&
                                                   requestedPath) {
            std::vector<std::string> strs;
            boost::split(strs, requestedPath, boost::is_any_of("/"));
            std::string objectPath;
            std::string interfaceName;
            std::string methodName;
            auto it = strs.begin();
            if (it == strs.end())
            {
                objectPath = "/";
            }
            while (it != strs.end())
            {
                // Check if segment contains ".".  If it does, it must be an
                // interface
                if (it->find(".") != std::string::npos)
                {
                    break;
                    // This check is necessary as the trailing slash gets
                    // parsed as part of our <path> specifier above, which
                    // causes the normal trailing backslash redirector to
                    // fail.
                }
                if (!it->empty())
                {
                    objectPath += "/" + *it;
                }
                it++;
            }
            if (it != strs.end())
            {
                interfaceName = *it;
                it++;

                // after interface, we might have a method name
                if (it != strs.end())
                {
                    methodName = *it;
                    it++;
                }
            }
            if (it != strs.end())
            {
                // if there is more levels past the method name, something
                // went wrong, return not found
                res.result(boost::beast::http::status::not_found);
                res.end();
                return;
            }
            if (interfaceName.empty())
            {
                std::shared_ptr<bmcweb::AsyncResp> asyncResp =
                    std::make_shared<bmcweb::AsyncResp>(res);

                crow::connections::systemBus->async_method_call(
                    [asyncResp, processName,
                     objectPath](const boost::system::error_code ec,
                                 const std::string& introspectXml) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR
                                << "Introspect call failed with error: "
                                << ec.message()
                                << " on process: " << processName
                                << " path: " << objectPath << "\n";
                            return;
                        }
                        tinyxml2::XMLDocument doc;

                        doc.Parse(introspectXml.c_str());
                        tinyxml2::XMLNode* pRoot =
                            doc.FirstChildElement("node");
                        if (pRoot == nullptr)
                        {
                            BMCWEB_LOG_ERROR << "XML document failed to parse "
                                             << processName << " " << objectPath
                                             << "\n";
                            asyncResp->res.jsonValue = {
                                {"status", "XML parse error"}};
                            asyncResp->res.result(boost::beast::http::status::
                                                      internal_server_error);
                            return;
                        }

                        BMCWEB_LOG_DEBUG << introspectXml;
                        asyncResp->res.jsonValue = {
                            {"status", "ok"},
                            {"bus_name", processName},
                            {"object_path", objectPath}};
                        nlohmann::json& interfacesArray =
                            asyncResp->res.jsonValue["interfaces"];
                        interfacesArray = nlohmann::json::array();
                        tinyxml2::XMLElement* interface =
                            pRoot->FirstChildElement("interface");

                        while (interface != nullptr)
                        {
                            const char* ifaceName =
                                interface->Attribute("name");
                            if (ifaceName != nullptr)
                            {
                                interfacesArray.push_back(
                                    {{"name", ifaceName}});
                            }

                            interface =
                                interface->NextSiblingElement("interface");
                        }
                    },
                    processName, objectPath,
                    "org.freedesktop.DBus.Introspectable", "Introspect");
            }
            else if (methodName.empty())
            {
                std::shared_ptr<bmcweb::AsyncResp> asyncResp =
                    std::make_shared<bmcweb::AsyncResp>(res);

                crow::connections::systemBus->async_method_call(
                    [asyncResp, processName, objectPath,
                     interfaceName](const boost::system::error_code ec,
                                    const std::string& introspectXml) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR
                                << "Introspect call failed with error: "
                                << ec.message()
                                << " on process: " << processName
                                << " path: " << objectPath << "\n";
                            return;
                        }
                        tinyxml2::XMLDocument doc;

                        doc.Parse(introspectXml.data(), introspectXml.size());
                        tinyxml2::XMLNode* pRoot =
                            doc.FirstChildElement("node");
                        if (pRoot == nullptr)
                        {
                            BMCWEB_LOG_ERROR << "XML document failed to parse "
                                             << processName << " " << objectPath
                                             << "\n";
                            asyncResp->res.result(boost::beast::http::status::
                                                      internal_server_error);
                            return;
                        }
                        asyncResp->res.jsonValue = {
                            {"status", "ok"},
                            {"bus_name", processName},
                            {"interface", interfaceName},
                            {"object_path", objectPath}};

                        nlohmann::json& methodsArray =
                            asyncResp->res.jsonValue["methods"];
                        methodsArray = nlohmann::json::array();

                        nlohmann::json& signalsArray =
                            asyncResp->res.jsonValue["signals"];
                        signalsArray = nlohmann::json::array();

                        nlohmann::json& propertiesObj =
                            asyncResp->res.jsonValue["properties"];
                        propertiesObj = nlohmann::json::object();

                        // if we know we're the only call, build the
                        // json directly
                        tinyxml2::XMLElement* interface =
                            pRoot->FirstChildElement("interface");
                        while (interface != nullptr)
                        {
                            const char* ifaceName =
                                interface->Attribute("name");

                            if (ifaceName != nullptr &&
                                ifaceName == interfaceName)
                            {
                                break;
                            }

                            interface =
                                interface->NextSiblingElement("interface");
                        }
                        if (interface == nullptr)
                        {
                            // if we got to the end of the list and
                            // never found a match, throw 404
                            asyncResp->res.result(
                                boost::beast::http::status::not_found);
                            return;
                        }

                        tinyxml2::XMLElement* methods =
                            interface->FirstChildElement("method");
                        while (methods != nullptr)
                        {
                            nlohmann::json argsArray = nlohmann::json::array();
                            tinyxml2::XMLElement* arg =
                                methods->FirstChildElement("arg");
                            while (arg != nullptr)
                            {
                                nlohmann::json thisArg;
                                for (const char* fieldName :
                                     std::array<const char*, 3>{
                                         "name", "direction", "type"})
                                {
                                    const char* fieldValue =
                                        arg->Attribute(fieldName);
                                    if (fieldValue != nullptr)
                                    {
                                        thisArg[fieldName] = fieldValue;
                                    }
                                }
                                argsArray.push_back(std::move(thisArg));
                                arg = arg->NextSiblingElement("arg");
                            }

                            const char* name = methods->Attribute("name");
                            if (name != nullptr)
                            {
                                std::string uri;
                                uri.reserve(14 + processName.size() +
                                            objectPath.size() +
                                            interfaceName.size() +
                                            strlen(name));
                                uri += "/bus/system/";
                                uri += processName;
                                uri += objectPath;
                                uri += "/";
                                uri += interfaceName;
                                uri += "/";
                                uri += name;
                                methodsArray.push_back({{"name", name},
                                                        {"uri", std::move(uri)},
                                                        {"args", argsArray}});
                            }
                            methods = methods->NextSiblingElement("method");
                        }
                        tinyxml2::XMLElement* signals =
                            interface->FirstChildElement("signal");
                        while (signals != nullptr)
                        {
                            nlohmann::json argsArray = nlohmann::json::array();

                            tinyxml2::XMLElement* arg =
                                signals->FirstChildElement("arg");
                            while (arg != nullptr)
                            {
                                const char* name = arg->Attribute("name");
                                const char* type = arg->Attribute("type");
                                if (name != nullptr && type != nullptr)
                                {
                                    argsArray.push_back({
                                        {"name", name},
                                        {"type", type},
                                    });
                                }
                                arg = arg->NextSiblingElement("arg");
                            }
                            const char* name = signals->Attribute("name");
                            if (name != nullptr)
                            {
                                signalsArray.push_back(
                                    {{"name", name}, {"args", argsArray}});
                            }

                            signals = signals->NextSiblingElement("signal");
                        }

                        tinyxml2::XMLElement* property =
                            interface->FirstChildElement("property");
                        while (property != nullptr)
                        {
                            const char* name = property->Attribute("name");
                            const char* type = property->Attribute("type");
                            if (type != nullptr && name != nullptr)
                            {
                                sdbusplus::message::message m =
                                    crow::connections::systemBus
                                        ->new_method_call(processName.c_str(),
                                                          objectPath.c_str(),
                                                          "org.freedesktop."
                                                          "DBus."
                                                          "Properties",
                                                          "Get");
                                m.append(interfaceName, name);
                                nlohmann::json& propertyItem =
                                    propertiesObj[name];
                                crow::connections::systemBus->async_send(
                                    m, [&propertyItem, asyncResp](
                                           boost::system::error_code& e,
                                           sdbusplus::message::message& msg) {
                                        if (e)
                                        {
                                            return;
                                        }

                                        convertDBusToJSON("v", msg,
                                                          propertyItem);
                                    });
                            }
                            property = property->NextSiblingElement("property");
                        }
                    },
                    processName, objectPath,
                    "org.freedesktop.DBus.Introspectable", "Introspect");
            }
            else
            {
                if (req.method() != boost::beast::http::verb::post)
                {
                    res.result(boost::beast::http::status::not_found);
                    res.end();
                    return;
                }

                nlohmann::json requestDbusData =
                    nlohmann::json::parse(req.body, nullptr, false);

                if (requestDbusData.is_discarded())
                {
                    res.result(boost::beast::http::status::bad_request);
                    res.end();
                    return;
                }
                if (!requestDbusData.is_array())
                {
                    res.result(boost::beast::http::status::bad_request);
                    res.end();
                    return;
                }
                auto transaction = std::make_shared<InProgressActionData>(res);

                transaction->path = objectPath;
                transaction->methodName = methodName;
                transaction->arguments = std::move(requestDbusData);

                findActionOnInterface(transaction, processName);
            }
        });
}
} // namespace openbmc_mapper
} // namespace crow
