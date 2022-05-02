#pragma once

#include "http_request.hpp"
#include "openbmc_dbus_rest.hpp"

#include <async_resp.hpp>
#include <utils/collection.hpp>

#include <vector>

// Provides strongly-typed interfaces to dbus for abstraction and help with unit
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
struct DbusMethodAddr
{
    DbusMethodAddr(const std::string& service, const std::string& objpath,
                   const char* iface, const std::string& method) :
        service(service),
        objpath(objpath), iface(iface), method(method)
    {}

    DbusMethodAddr(const ResolvedEntity& resolvedEntity,
                   const std::string& method) :
        service(resolvedEntity.service),
        objpath(resolvedEntity.object), iface(resolvedEntity.interface),
        method(method)
    {}

    std::string service;
    std::string objpath;
    const char* iface;
    std::string method;
};

struct ObjectMapperGetSubTreeParams
{
    int depth = 0;
    std::string subtree;
    std::vector<const char*> interfaces;
};

using ObjectMapperSubTreeCallback =
    std::function<void(const boost::system::error_code,
                       const crow::openbmc_mapper::GetSubTreeType&)>;

// Wrapper around accesses to objectMapper D-BUS object.
// TODO: move to dbus_utility.hpp at some point.
class ObjectMapperInterface
{
  public:
    virtual void getSubTree(const DbusMethodAddr& addr,
                            const ObjectMapperGetSubTreeParams& params,
                            const ObjectMapperSubTreeCallback& asyncHandler)
    {
        crow::connections::systemBus->async_method_call(
            asyncHandler, addr.service, addr.objpath, addr.iface, addr.method,
            params.subtree.c_str(), params.depth, params.interfaces);
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

struct GoogleServiceAsyncResp
{
    GoogleServiceAsyncResp(
        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
        const std::shared_ptr<ObjectMapperInterface>& objMapper,
        const std::shared_ptr<RedfishUtilWrapper>& rfUtils,
        const std::shared_ptr<HothInterface>& hothInterface) :
        asyncResp(asyncResp),
        objMapper(objMapper), rfUtils(rfUtils), hothInterface(hothInterface)
    {}

    explicit GoogleServiceAsyncResp(
        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) :
        GoogleServiceAsyncResp(asyncResp,
                               std::make_shared<ObjectMapperInterface>(),
                               std::make_shared<RedfishUtilWrapper>(),
                               std::make_shared<HothInterface>())
    {}

    std::shared_ptr<bmcweb::AsyncResp> asyncResp;
    std::shared_ptr<ObjectMapperInterface> objMapper;
    std::shared_ptr<RedfishUtilWrapper> rfUtils;
    std::shared_ptr<HothInterface> hothInterface;
};

} // namespace google_api
} // namespace crow
