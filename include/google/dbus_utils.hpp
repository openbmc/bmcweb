#pragma once

#include <utils/collection.hpp>
#include <vector>

// Provides stongly-typed interfaces to dbus for abstraction and help with unit testing.

namespace crow
{
namespace google_api
{

// Helper struct to identify a resolved D-Bus object interface
struct ResolvedEntity {
    std::string id;
    std::string service;
    std::string object;
    const char* interface;
};

// Helper struct to identify a resolved D-Bus object interface method
class DbusMethodAddr {
    public:
        DbusMethodAddr(const std::string& service_name,
            const std::string& objectPath,
            const char* interface,
            const std::string& methodName) : service(service_name), objpath(objectPath), iface(interface), method(methodName) {}
        std::string service;
        std::string objpath;
        const char* iface;
        std::string method;
};

static DbusMethodAddr toDbusMethodAddr(const ResolvedEntity& resolvedEntity, const std::string& method) {
    return DbusMethodAddr(resolvedEntity.service, resolvedEntity.object, resolvedEntity.interface, method);
}

class ObjectMapperGetSubTreeParams {
    public:
        int depth;
        const char* subtree;
        std::vector<const char*> interfaces;
};

using ObjectMapperSubTreeCallback = std::function<void(const boost::system::error_code, const crow::openbmc_mapper::GetSubTreeType&)>;

// Wrapper around accesses to objectMapper D-BUS object.
class ObjectMapperInterface {
    public:
       virtual void getSubTree(
                    const DbusMethodAddr& addr,
                    const ObjectMapperGetSubTreeParams& params,
                    const ObjectMapperSubTreeCallback& async_handler) {
            crow::connections::systemBus->async_method_call(
                async_handler, 
                addr.service, addr.objpath, addr.iface, addr.method, params.subtree, params.depth, params.interfaces);
        }
        virtual ~ObjectMapperInterface() = default;
};

using HothSendCommandCallback = std::function<void(const boost::system::error_code, std::vector<uint8_t>&)>;

// Wrapper around accesses to Hoth D-BUS object.
class HothInterface {
    public:
       virtual void sendHostCommand(
                    const DbusMethodAddr& addr,
                    const std::vector<uint8_t>& command,
                    const HothSendCommandCallback& async_handler) {
            crow::connections::systemBus->async_method_call(
                async_handler, 
                addr.service, addr.objpath, addr.iface, addr.method, command);
        }
        virtual ~HothInterface() = default;
};

// Wrapper around other Redfish utility methods which rely on D-BUS access.
class RedfishUtilWrapper {
    public:
        virtual void populateCollectionMembers(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& collectionPath,
                         const std::vector<const char*>& interfaces,
                         const char* subtree) {       
            redfish::collection_util::getCollectionMembers(
                asyncResp, collectionPath,
                interfaces, subtree);
        }
        virtual ~RedfishUtilWrapper() = default;
};
} // namespace google_api
} // namespace crow
