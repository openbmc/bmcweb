#pragma once

#include "async_resp.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "redfishoemrule.hpp"
#include "utils/error_code.hpp"
#include "utils/json_utils.hpp"
#include "utils/query_param.hpp"
#include "verb.hpp"

#include <nlohmann/json.hpp>

#include <memory>
#include <optional>
#include <string_view>
#include <vector>

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage, clang-diagnostic-unused-macros)
#define BMCWEB_OEM_ROUTE(router, url)                                          \
    router.template newOemRule<crow::utility::getParameterTag(url)>(url)

namespace redfish
{
class OemRouter
{
  public:
    OemRouter() = default;

    template <uint64_t NumArgs>
    auto& newOemRule(const std::string& rule)
    {
        if constexpr (NumArgs == 0)
        {
            using RuleT = OemRule<>;
            std::unique_ptr<RuleT> ruleObject = std::make_unique<RuleT>(rule);
            RuleT* ptr = ruleObject.get();
            allRules.emplace_back(std::move(ruleObject));
            return *ptr;
        }
        else if constexpr (NumArgs == 1)
        {
            using RuleT = OemRule<std::string>;
            std::unique_ptr<RuleT> ruleObject = std::make_unique<RuleT>(rule);
            RuleT* ptr = ruleObject.get();
            allRules.emplace_back(std::move(ruleObject));
            return *ptr;
        }
        else if constexpr (NumArgs == 2)
        {
            using RuleT = OemRule<std::string, std::string>;
            std::unique_ptr<RuleT> ruleObject = std::make_unique<RuleT>(rule);
            RuleT* ptr = ruleObject.get();
            allRules.emplace_back(std::move(ruleObject));
            return *ptr;
        }
        else if constexpr (NumArgs == 3)
        {
            using RuleT = OemRule<std::string, std::string, std::string>;
            std::unique_ptr<RuleT> ruleObject = std::make_unique<RuleT>(rule);
            RuleT* ptr = ruleObject.get();
            allRules.emplace_back(std::move(ruleObject));
            return *ptr;
        }
        else if constexpr (NumArgs == 4)
        {
            using RuleT =
                OemRule<std::string, std::string, std::string, std::string>;
            std::unique_ptr<RuleT> ruleObject = std::make_unique<RuleT>(rule);
            RuleT* ptr = ruleObject.get();
            allRules.emplace_back(std::move(ruleObject));
            return *ptr;
        }
        else
        {
            using RuleT = OemRule<std::string, std::string, std::string,
                                  std::string, std::string>;
            std::unique_ptr<RuleT> ruleObject = std::make_unique<RuleT>(rule);
            RuleT* ptr = ruleObject.get();
            allRules.emplace_back(std::move(ruleObject));
            return *ptr;
        }
        static_assert(NumArgs <= 5, "Max number of args supported is 5");
    }

    struct PerMethod
    {
        std::vector<OemBaseRule*> rules;
        crow::Trie trie;
        // rule index 0 has special meaning; preallocate it to avoid
        // duplication.
        PerMethod() : rules(1) {}

        void internalAdd(std::string_view rule, OemBaseRule* ruleObject)
        {
            rules.emplace_back(ruleObject);
            trie.add(rule, static_cast<unsigned>(rules.size() - 1U));
            // request to /resource#frag url matches /resource/#frag
            size_t hashPos = rule.find('#');
            if (hashPos != std::string_view::npos)
            {
                std::string url(rule.substr(0, hashPos));
                url += '/';
                url += rule.substr(hashPos);
                std::string_view fragRule = url;
                trie.add(fragRule, static_cast<unsigned>(rules.size() - 1U));
            }
        }
    };

    void internalAddRuleObject(const std::string& rule, OemBaseRule* ruleObject)
    {
        if (ruleObject == nullptr)
        {
            return;
        }
        for (size_t method = 0; method <= maxVerbIndex; method++)
        {
            size_t methodBit = 1 << method;
            if ((ruleObject->methodsBitfield & methodBit) > 0U)
            {
                perMethods[method].internalAdd(rule, ruleObject);
            }
        }
    }

    void validate()
    {
        for (std::unique_ptr<OemBaseRule>& rule : allRules)
        {
            if (rule)
            {
                rule->validate();
                internalAddRuleObject(rule->rule, rule.get());
            }
        }
        for (PerMethod& perMethod : perMethods)
        {
            perMethod.trie.validate();
        }
    }

    struct FindRoute
    {
        std::vector<OemBaseRule*> fragmentRules;
        std::vector<std::string> params;
    };

    struct FindRouteResponse
    {
        FindRoute route;
    };

    static FindRoute findRouteByPerMethod(std::string_view url,
                                          const PerMethod& perMethod)
    {
        FindRoute route;

        crow::Trie::FindResult found = perMethod.trie.find(url);
        if (found.ruleIndex >= perMethod.rules.size())
        {
            throw std::runtime_error("Trie internal structure corrupted!");
        }
        route.params = std::move(found.params);
        for (auto fragmentRuleIndex : found.fragmentRuleIndexes)
        {
            route.fragmentRules.emplace_back(
                perMethod.rules[fragmentRuleIndex]);
        }
        return route;
    }

    FindRouteResponse findRoute(const crow::Request& req) const
    {
        FindRouteResponse findRoute;
        std::optional<HttpVerb> verb = httpVerbFromBoost(req.method());
        if (!verb)
        {
            return findRoute;
        }
        size_t reqMethodIndex = static_cast<size_t>(*verb);
        if (reqMethodIndex >= perMethods.size())
        {
            return findRoute;
        }

        FindRoute route = findRouteByPerMethod(req.url().encoded_path(),
                                               perMethods[reqMethodIndex]);
        if (!route.fragmentRules.empty())
        {
            findRoute.route = route;
        }
        else
        {
            BMCWEB_LOG_DEBUG(
                "No fragments for for url {}, method {}",
                req.url().encoded_path(),
                httpVerbToString(static_cast<HttpVerb>(reqMethodIndex)));
        }

        return findRoute;
    }

    void handleOemGet(const std::shared_ptr<crow::Request>& req,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) const
    {
        BMCWEB_LOG_DEBUG("Checking OEM routes");
        FindRouteResponse foundRoute = findRoute(*req);
        std::vector<OemBaseRule*> fragments =
            std::move(foundRoute.route.fragmentRules);
        std::vector<std::string> params = std::move(foundRoute.route.params);
        if (!fragments.empty())
        {
            std::function<void(crow::Response&)> handler =
                asyncResp->res.releaseCompleteRequestHandler();
            auto multiResp = std::make_shared<bmcweb::AsyncResp>();
            multiResp->res.setCompleteRequestHandler(std::move(handler));

            // Copy so that they exists when completion handler is called.
            auto uriFragments =
                std::make_shared<std::vector<OemBaseRule*>>(fragments);
            auto uriParams = std::make_shared<std::vector<std::string>>(params);

            asyncResp->res.setCompleteRequestHandler(std::bind_front(
                query_param::MultiAsyncResp::startMultiFragmentGet,
                std::make_shared<crow::Request>(*req), multiResp, uriFragments,
                uriParams));
        }
        else
        {
            BMCWEB_LOG_DEBUG("No OEM routes found");
        }
    }

    void debugPrint()
    {
        for (size_t i = 0; i < perMethods.size(); i++)
        {
            BMCWEB_LOG_DEBUG("{}", httpVerbToString(static_cast<HttpVerb>(i)));
            perMethods[i].trie.debugPrint();
        }
    }

    std::vector<const std::string*> getRoutes(const std::string& parent)
    {
        std::vector<const std::string*> ret;

        for (const PerMethod& pm : perMethods)
        {
            std::vector<unsigned> x;
            pm.trie.findRouteIndexes(parent, x);
            for (unsigned index : x)
            {
                ret.push_back(&pm.rules[index]->rule);
            }
        }
        return ret;
    }

  private:
    std::array<PerMethod, static_cast<size_t>(HttpVerb::Max)> perMethods;
    std::vector<std::unique_ptr<OemBaseRule>> allRules;
};

inline OemRouter& getOemRouter()
{
    static OemRouter r;
    return r;
}

inline void oemRouterInit()
{
    getOemRouter().validate();
}

} // namespace redfish
