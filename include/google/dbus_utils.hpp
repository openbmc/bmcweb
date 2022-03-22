// Copyright 2022 Google LLC
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

#include "http_request.hpp"
#include "openbmc_dbus_rest.hpp"

#include <async_resp.hpp>
#include <utils/collection.hpp>

#include <vector>

// Provides stongly-typed interfaces to dbus for abstraction and help with unit
// testing.

namespace crow
{
namespace google_api
{

// Helper struct to identify a resolved D-Bus object interface
struct ResolvedEntity
{
    std::string id;
    std::string service;
    std::string object;
    const char* interface;
};

// Helper struct to identify a resolved D-Bus object interface method
class DbusMethodAddr
{
  public:
    std::string service;
    std::string objpath;
    const char* iface;
    std::string method;
};

static DbusMethodAddr toDbusMethodAddr(const ResolvedEntity& resolvedEntity,
                                       const std::string& method)
{
    return {.service = resolvedEntity.service,
            .objpath = resolvedEntity.object,
            .iface = resolvedEntity.interface,
            .method = method};
}

struct ObjectMapperGetSubTreeParams
{
    int depth{};
    const char* subtree{};
    std::vector<const char*> interfaces;
};

using ObjectMapperSubTreeCallback =
    std::function<void(const boost::system::error_code,
                       const crow::openbmc_mapper::GetSubTreeType&)>;

// Wrapper around accesses to objectMapper D-BUS object.
class ObjectMapperInterface
{
  public:
    virtual void getSubTree(const DbusMethodAddr& addr,
                            const ObjectMapperGetSubTreeParams& params,
                            const ObjectMapperSubTreeCallback& asyncHandler)
    {
        crow::connections::systemBus->async_method_call(
            asyncHandler, addr.service, addr.objpath, addr.iface, addr.method,
            params.subtree, params.depth, params.interfaces);
    }
    virtual ~ObjectMapperInterface() = default;
    ObjectMapperInterface() = default;
    ObjectMapperInterface(const ObjectMapperInterface&) = delete;
    ObjectMapperInterface& operator=(const ObjectMapperInterface&) = delete;
    ObjectMapperInterface(ObjectMapperInterface&&) = delete;
    ObjectMapperInterface& operator=(ObjectMapperInterface&&) = delete;
};

using HothSendCommandCallback =
    std::function<void(const boost::system::error_code, std::vector<uint8_t>&)>;

// Wrapper around accesses to Hoth D-BUS object.
class HothInterface
{
  public:
    virtual void sendHostCommand(const DbusMethodAddr& addr,
                                 const std::vector<uint8_t>& command,
                                 const HothSendCommandCallback& asyncHandler)
    {
        crow::connections::systemBus->async_method_call(
            asyncHandler, addr.service, addr.objpath, addr.iface, addr.method,
            command);
    }
    virtual ~HothInterface() = default;
    HothInterface() = default;
    HothInterface(const HothInterface&) = delete;
    HothInterface& operator=(const HothInterface&) = delete;
    HothInterface(HothInterface&&) = delete;
    HothInterface& operator=(HothInterface&&) = delete;
};

// Wrapper around other Redfish utility methods which rely on D-BUS access.
class RedfishUtilWrapper
{
  public:
    virtual void populateCollectionMembers(
        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
        const std::string& collectionPath,
        const std::vector<const char*>& interfaces, const char* subtree)
    {
        redfish::collection_util::getCollectionMembers(
            asyncResp, collectionPath, interfaces, subtree);
    }
    virtual ~RedfishUtilWrapper() = default;   
    RedfishUtilWrapper() = default;
    RedfishUtilWrapper(const RedfishUtilWrapper&) = delete;
    RedfishUtilWrapper& operator=(const RedfishUtilWrapper&) = delete;
    RedfishUtilWrapper(RedfishUtilWrapper&&) = delete;
    RedfishUtilWrapper& operator=(RedfishUtilWrapper&&) = delete;
};
} // namespace google_api
} // namespace crow
