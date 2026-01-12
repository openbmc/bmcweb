#pragma once

#include <initializer_list>
#include <optional>

namespace crow
{
template <typename T>
struct PrivilegeParameterTraits
{
    using self_t = T;

    self_t& privileges(
        const std::initializer_list<std::initializer_list<const char*>>& p)
    {
        self_t* self = static_cast<self_t*>(this);
        for (const std::initializer_list<const char*>& privilege : p)
        {
            self->privilegesSet.emplace_back(privilege);
        }
        return *self;
    }

    template <size_t N, typename... MethodArgs>
    self_t& privileges(const std::array<redfish::Privileges, N>& p)
    {
        self_t* self = static_cast<self_t*>(this);
        for (const redfish::Privileges& privilege : p)
        {
            self->privilegesSet.emplace_back(privilege);
        }
        return *self;
    }
};
} // namespace crow
