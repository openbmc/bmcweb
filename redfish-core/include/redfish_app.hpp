#pragma once

#include "app.hpp"
#include "privileges.hpp"
#include "redfish_rule.hpp"
#include "registries/privilege_registry.hpp"
#include "utility.hpp"

namespace redfish
{

class RedfishApp
{
  public:
    explicit RedfishApp(crow::App& appIn) :
        app(appIn), router(appIn.getRouter()){};

    template <uint64_t N>
    auto& route(std::string&& rule)
    {
        using RuleT = typename crow::black_magic::Arguments<N>::type::template rebind<
            RedfishRule>;
        std::unique_ptr<RuleT> ruleObject = std::make_unique<RuleT>(rule);
        RuleT* ptr = ruleObject.get();
        router.addNewRule(std::move(ruleObject));
        return *ptr;
    }

    crow::App& getBaseApp()
    {
        return app;
    }

  private:
    crow::App& app;
    crow::Router& router;
};
} // namespace redfish