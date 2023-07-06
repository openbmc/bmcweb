#pragma once

#include "sserule.hpp"
#include "websocketrule.hpp"

#include <boost/beast/http/verb.hpp>

#include <initializer_list>
#include <optional>

namespace crow
{
template <typename T>
struct RuleParameterTraits
{
    using self_t = T;
    WebSocketRule& websocket()
    {
        auto* self = static_cast<self_t*>(this);
        auto* p = new WebSocketRule(self->rule);
        p->privilegesSet = self->privilegesSet;
        self->ruleToUpgrade.reset(p);
        return *p;
    }

    SseSocketRule& serverSentEvent()
    {
        auto* self = static_cast<self_t*>(this);
        auto* p = new SseSocketRule(self->rule);
        self->ruleToUpgrade.reset(p);
        return *p;
    }

    self_t& methods(boost::beast::http::verb method)
    {
        auto* self = static_cast<self_t*>(this);
        std::optional<HttpVerb> verb = httpVerbFromBoost(method);
        if (verb)
        {
            self->methodsBitfield = 1U << static_cast<size_t>(*verb);
        }
        return *self;
    }

    template <typename... MethodArgs>
    self_t& methods(boost::beast::http::verb method, MethodArgs... argsMethod)
    {
        auto* self = static_cast<self_t*>(this);
        methods(argsMethod...);
        std::optional<HttpVerb> verb = httpVerbFromBoost(method);
        if (verb)
        {
            self->methodsBitfield |= 1U << static_cast<size_t>(*verb);
        }
        return *self;
    }

    self_t& notFound()
    {
        auto* self = static_cast<self_t*>(this);
        self->methodsBitfield = 1U << notFoundIndex;
        return *self;
    }

    self_t& methodNotAllowed()
    {
        auto* self = static_cast<self_t*>(this);
        self->methodsBitfield = 1U << methodNotAllowedIndex;
        return *self;
    }

    self_t& privileges(
        const std::initializer_list<std::initializer_list<const char*>>& p)
    {
        auto* self = static_cast<self_t*>(this);
        for (const std::initializer_list<const char*>& privilege : p)
        {
            self->privilegesSet.emplace_back(privilege);
        }
        return *self;
    }

    template <size_t N, typename... MethodArgs>
    self_t& privileges(const std::array<redfish::Privileges, N>& p)
    {
        auto* self = static_cast<self_t*>(this);
        for (const redfish::Privileges& privilege : p)
        {
            self->privilegesSet.emplace_back(privilege);
        }
        return *self;
    }
};
} // namespace crow
