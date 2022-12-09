#pragma once

#include "registries/privilege_registry.hpp"
#include "routing.hpp"

namespace redfish
{

template <typename... Args>
class RedfishRule : public crow::TaggedRule<Args...>
{
    using self_t = RedfishRule<Args...>;

  public:
    explicit RedfishRule(const std::string& ruleIn) :
        crow::TaggedRule<Args...>(ruleIn){}

    self_t& entityTag(redfish::privileges::EntityTag entityTag)
    {
        self_t* self = static_cast<self_t*>(this);
        self->tag = entityTag;
        return *self;
    };

  private:
    redfish::privileges::EntityTag tag = redfish::privileges::EntityTag::none;
};
} // namespace redfish