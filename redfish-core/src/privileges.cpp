#include "privileges.hpp"

#include "http_verbs.hpp"
#include "registries/privilege_registry.hpp"

#include <string>
#include <string_view>

namespace redfish
{

const int Privileges::basePrivilegeCount = privileges::basePrivileges.size();

std::vector<std::string> Privileges::privilegeNames = {
    privileges::basePrivileges.begin(), privileges::basePrivileges.end()};

constexpr Mappings getBaseMappings()
{
    Mappings mappings;
    // Note: the order of methods must match |http::allRedfishMethods|
    constexpr std::array<const uint64_t*, http::allRedfishMethods.size()>
        basePrivilegesBitmapPtr = {
            &privileges::deleteBasePrivilegesBitmaps[0],
            &privileges::getBasePrivilegesBitmaps[0],
            &privileges::headBasePrivilegesBitmaps[0],
            &privileges::postBasePrivilegesBitmaps[0],
            &privileges::putBasePrivilegesBitmaps[0],
            &privileges::patchBasePrivilegesBitmaps[0],
        };
    constexpr std::array<size_t, http::allRedfishMethods.size()>
        basePrivilegesBitmapTotalLength = {
            privileges::deleteBasePrivilegesBitmaps.size(),
            privileges::getBasePrivilegesBitmaps.size(),
            privileges::headBasePrivilegesBitmaps.size(),
            privileges::postBasePrivilegesBitmaps.size(),
            privileges::putBasePrivilegesBitmaps.size(),
            privileges::patchBasePrivilegesBitmaps.size(),
        };
    constexpr std::array<const uint64_t*, http::allRedfishMethods.size()>
        basePrivilegesLengthPtr = {
            &privileges::deleteBasePrivilegesLength[0],
            &privileges::getBasePrivilegesLength[0],
            &privileges::headBasePrivilegesLength[0],
            &privileges::postBasePrivilegesLength[0],
            &privileges::putBasePrivilegesLength[0],
            &privileges::patchBasePrivilegesLength[0],
        };
    for (size_t k = 0; k < http::allRedfishMethods.size(); ++k)
    {
        size_t privilegeIndex = 0;
        boost::beast::http::verb method = http::allRedfishMethods[k];
        for (size_t i = 0; i < privileges::entities.size(); ++i)
        {
            for (size_t j = 0; j < basePrivilegesLengthPtr[k][i]; ++j)
            {
                mappings[i][http::boostVerbIndex[static_cast<size_t>(method)]]
                    .push_back(Privileges(
                        basePrivilegesBitmapPtr[k][privilegeIndex++]));
            }
        }
        if (privilegeIndex != basePrivilegesBitmapTotalLength[k])
        {
            BMCWEB_LOG_CRITICAL
                << "all privileges in BasePrivileges shall be used!";
        }
    }
    return mappings;
}

Mappings Privileges::mappings = getBaseMappings();

Privileges::Privileges(std::initializer_list<const char*> privilegeList)
{
    for (const char* privilege : privilegeList)
    {
        if (!setSinglePrivilege(privilege))
        {
            BMCWEB_LOG_CRITICAL << "Unable to set privilege " << privilege
                                << "in constructor";
        }
    }
}

bool Privileges::setSinglePrivilege(const std::string_view privilege)
{
    for (size_t searchIndex = 0; searchIndex < privilegeNames.size();
         searchIndex++)
    {
        if (privilege == privilegeNames[searchIndex])
        {
            privilegeBitset.set(searchIndex);
            return true;
        }
    }

    return false;
}

bool Privileges::resetSinglePrivilege(std::string_view privilege)
{
    for (size_t searchIndex = 0; searchIndex < privilegeNames.size();
         searchIndex++)
    {
        if (privilege == privilegeNames[searchIndex])
        {
            privilegeBitset.reset(searchIndex);
            return true;
        }
    }
    return false;
}

std::vector<std::string> Privileges::getAllActivePrivilegeNames() const
{
    std::vector<std::string> activePrivileges =
        getActivePrivilegeNames(PrivilegeType::BASE);
    std::vector<std::string> oemPrivileges =
        getActivePrivilegeNames(PrivilegeType::OEM);
    activePrivileges.insert(activePrivileges.end(),
                            std::make_move_iterator(oemPrivileges.begin()),
                            std::make_move_iterator(oemPrivileges.end()));
    return activePrivileges;
}

std::vector<std::string>
    Privileges::getActivePrivilegeNames(const PrivilegeType type) const
{
    std::vector<std::string> activePrivileges;

    size_t searchIndex = 0;
    size_t endIndex = basePrivilegeCount;
    if (type == PrivilegeType::OEM)
    {
        searchIndex = basePrivilegeCount;
        endIndex = privilegeNames.size();
    }

    for (; searchIndex < endIndex; searchIndex++)
    {
        if (privilegeBitset.test(searchIndex))
        {
            activePrivileges.emplace_back(privilegeNames[searchIndex]);
        }
    }

    return activePrivileges;
}

nlohmann::json Privileges::getOperationMap(privileges::EntityTag tag)
{
    size_t tagIndex = static_cast<size_t>(tag);
    nlohmann::json::object_t operationMap = {};
    for (size_t i = 0; i < http::allRedfishMethods.size(); ++i)
    {
        boost::beast::http::verb method = http::allRedfishMethods[i];
        size_t methodIndex = http::boostVerbIndex[static_cast<size_t>(method)];
        nlohmann::json::array_t privilegesOrArr = {};
        for (const Privileges& privileges : mappings[tagIndex][methodIndex])
        {
            nlohmann::json::object_t privilegesObject = {};
            for (const std::string& str :
                 privileges.getAllActivePrivilegeNames())
            {
                privilegesObject["Privilege"].emplace_back(str);
            }
            // NoAuth on ServiceRoot has been implemented by this service.
            if (privilegesObject.empty())
            {
                privilegesObject["Privilege"].emplace_back("NoAuth");
            }
            privilegesOrArr.emplace_back(std::move(privilegesObject));
        }
        operationMap[http::allRedfishMethodsStr[i]] =
            std::move(privilegesOrArr);
    }
    return operationMap;
}

} // namespace redfish