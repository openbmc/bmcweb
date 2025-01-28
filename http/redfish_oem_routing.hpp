#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "routing.hpp"
#include "routing/redfishoemrule.hpp"
#include "utility.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/error_code.hpp"
#include "utils/json_utils.hpp"
#include "verb.hpp"
#include "websocket.hpp"

#include <boost/container/flat_map.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/url.hpp>
#include <boost/url/url_view.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <optional>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace crow
{

using namespace ::redfish;

class MultiRouteResp : public std::enable_shared_from_this<MultiRouteResp>
{
  public:
    explicit MultiRouteResp(std::shared_ptr<bmcweb::AsyncResp> finalResIn) :
        finalRes(std::move(finalResIn))
    {}

    void addAwaitingResponse(const std::shared_ptr<bmcweb::AsyncResp>& res,
                             const nlohmann::json::json_pointer& finalLocation)
    {
        res->res.setCompleteRequestHandler(std::bind_front(
            placeResultStatic, shared_from_this(), finalLocation));
    }

    void placeResult(const nlohmann::json::json_pointer& locationToPlace,
                     crow::Response& res)
    {
        BMCWEB_LOG_DEBUG("got fragment response {}", logPtr(&res));
        propogateError(finalRes->res, res);
        if (!res.jsonValue.is_object() || res.jsonValue.empty())
        {
            return;
        }
        nlohmann::json& finalObj = finalRes->res.jsonValue[locationToPlace];
        finalObj = std::move(res.jsonValue);
    }

    static void handleGetFragmentRules(
        const std::shared_ptr<crow::Request>& req,
        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
        const std::shared_ptr<std::vector<RfOemBaseRule*>>& fragments,
        const std::shared_ptr<std::vector<std::string>>& params,
        const crow::Response& resIn)
    {
        asyncResp->res.jsonValue = resIn.jsonValue;
        auto multi = std::make_shared<MultiRouteResp>(asyncResp);
        for (auto* fragment : *fragments)
        {
            RfOemBaseRule& fragmentRule = *fragment;
            auto rsp = std::make_shared<bmcweb::AsyncResp>();
            BMCWEB_LOG_DEBUG("Matched fragment GET rule '{}' {} / {}",
                             fragmentRule.rule, req->methodString(),
                             fragmentRule.getMethods());
            BMCWEB_LOG_DEBUG(
                "Handling fragment rules: setting completion handler on {}",
                logPtr(&rsp->res));
            auto jsonFragmentPtr =
                redfish::json_util::createJsonPointerFromFragment(
                    fragmentRule.rule);
            if (jsonFragmentPtr)
            {
                multi->addAwaitingResponse(rsp, *jsonFragmentPtr);
                fragmentRule.handle(*req, rsp, *params);
            }
        }
    }


    static void handlePatchFragmentRules(
        const std::shared_ptr<crow::Request>& req,
        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
        const std::shared_ptr<std::vector<RfOemBaseRule*>>& fragments,
        const std::shared_ptr<std::vector<std::string>>& params,
        nlohmann::json::object_t& payload,
        const crow::Response& resIn)
    {
        asyncResp->res.jsonValue = resIn.jsonValue;
        auto multi = std::make_shared<MultiRouteResp>(asyncResp);
        auto& fragRules = *fragments;
        
        std::vector<std::optional<nlohmann::json::object_t>> jsonObjects;
        std::vector<redfish::json_util::PerUnpack> objectStorage(fragRules.size());
        std::span<redfish::json_util::PerUnpack> jsonKeyValuePair(objectStorage);
        jsonObjects.resize(fragRules.size());
        std::vector<nlohmann::json::json_pointer> jsonPointers;

        for (size_t i = 0; i < fragRules.size(); ++i)
        {
            auto& fragmentRule = *(fragRules[i]);
            const auto& jsonPointer = redfish::json_util::createJsonPointerFromFragment(
                fragmentRule.rule);
            if (jsonPointer) 
            {
                std::string_view key = (*jsonPointer).to_string();
                if (!key.empty() && key[0] == '/')
                {
                    key = key.substr(key.find('/', 1) + 1);
                }
                BMCWEB_LOG_DEBUG("Adding OEM fragment key: {}", key);
                jsonKeyValuePair[i] = redfish::json_util::PerUnpack{key, &jsonObjects[i]};
                jsonPointers.emplace_back(*jsonPointer);
            }    
            else
            {
                BMCWEB_LOG_DEBUG("Failed to get fragment rule JSON pointer");
                return;
            }
        }

        readJsonHelperObject(payload, asyncResp->res, jsonKeyValuePair);

        for (size_t i = 0; i < fragRules.size(); ++i)
        {
            // We have json object whcih matches the key in the rule 
            if (jsonObjects[i])
            {
                auto&  fragmentRule = *(fragRules[i]);
                BMCWEB_LOG_DEBUG("Matched fragment PATCH rule '{}' {} / {}",
                                fragmentRule.rule, req->methodString(),
                                fragmentRule.getMethods());
                auto rsp = std::make_shared<bmcweb::AsyncResp>();
                BMCWEB_LOG_DEBUG(
                    "Handling fragment rules: setting completion handler on {}",
                    logPtr(&rsp->res));
                multi->addAwaitingResponse(rsp, jsonPointers[i]);         
                fragmentRule.handle(*req, rsp, *params, *jsonObjects[i]);
            }
            else
            {
                BMCWEB_LOG_DEBUG("Patch object not found for key: {}", jsonPointers[i].to_string());
            }
        }
    }

  private:
    static void
        placeResultStatic(const std::shared_ptr<MultiRouteResp>& multi,
                          const nlohmann::json::json_pointer& locationToPlace,
                          crow::Response& res)
    {
        multi->placeResult(locationToPlace, res);
    }

    std::shared_ptr<bmcweb::AsyncResp> finalRes;
};

inline void handleMultiFragmentGetRouting(
    const std::shared_ptr<Request>& req,
    const std::vector<RfOemBaseRule*>& fragments,
    const std::vector<std::string>& params,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    std::function<void(crow::Response&)> handler =
        asyncResp->res.releaseCompleteRequestHandler();
    auto multiResp = std::make_shared<bmcweb::AsyncResp>();
    multiResp->res.setCompleteRequestHandler(std::move(handler));

    // Copy so that they exists when completion handler is called.
    auto uriFragments =
        std::make_shared<std::vector<RfOemBaseRule*>>(fragments);
    auto uriParams = std::make_shared<std::vector<std::string>>(params);

    asyncResp->res.setCompleteRequestHandler(
        std::bind_front(MultiRouteResp::handleGetFragmentRules,
                        std::make_shared<crow::Request>(*req), multiResp,
                        uriFragments, uriParams));
}


inline void handleMultiFragmentPatchRouting(
    const std::shared_ptr<Request>& req,
    const std::vector<RfOemBaseRule*>& fragments,
    const std::vector<std::string>& params,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const nlohmann::json::object_t& payload)
{
    std::function<void(crow::Response&)> handler =
        asyncResp->res.releaseCompleteRequestHandler();
    auto multiResp = std::make_shared<bmcweb::AsyncResp>();
    multiResp->res.setCompleteRequestHandler(std::move(handler));

    // Copy so that they exists when completion handler is called.
    auto uriFragments = std::make_shared<std::vector<RfOemBaseRule*>>(fragments);
    auto uriParams = std::make_shared<std::vector<std::string>>(params);

    asyncResp->res.setCompleteRequestHandler(
        std::bind_front(MultiRouteResp::handlePatchFragmentRules,
                        std::make_shared<crow::Request>(*req), multiResp,
                        uriFragments, uriParams, payload));
}

class RfOemRouter
{
  public:
    RfOemRouter() = default;

    template <uint64_t NumArgs>
    auto& newRfOemRule(const std::string& rule)
    {
        if constexpr (NumArgs == 0)
        {
            using RuleT = RfOemRule<>;
            std::unique_ptr<RuleT> ruleObject = std::make_unique<RuleT>(rule);
            RuleT* ptr = ruleObject.get();
            allRules.emplace_back(std::move(ruleObject));
            return *ptr;
        }
        else if constexpr (NumArgs == 1)
        {
            using RuleT = RfOemRule<std::string>;
            std::unique_ptr<RuleT> ruleObject = std::make_unique<RuleT>(rule);
            RuleT* ptr = ruleObject.get();
            allRules.emplace_back(std::move(ruleObject));
            return *ptr;
        }
        else if constexpr (NumArgs == 2)
        {
            using RuleT = RfOemRule<std::string, std::string>;
            std::unique_ptr<RuleT> ruleObject = std::make_unique<RuleT>(rule);
            RuleT* ptr = ruleObject.get();
            allRules.emplace_back(std::move(ruleObject));
            return *ptr;
        }
        else if constexpr (NumArgs == 3)
        {
            using RuleT = RfOemRule<std::string, std::string, std::string>;
            std::unique_ptr<RuleT> ruleObject = std::make_unique<RuleT>(rule);
            RuleT* ptr = ruleObject.get();
            allRules.emplace_back(std::move(ruleObject));
            return *ptr;
        }
        else if constexpr (NumArgs == 4)
        {
            using RuleT =
                RfOemRule<std::string, std::string, std::string, std::string>;
            std::unique_ptr<RuleT> ruleObject = std::make_unique<RuleT>(rule);
            RuleT* ptr = ruleObject.get();
            allRules.emplace_back(std::move(ruleObject));
            return *ptr;
        }
        else
        {
            using RuleT = RfOemRule<std::string, std::string, std::string,
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
        std::vector<RfOemBaseRule*> rules;
        Trie trie;
        // rule index 0 has special meaning; preallocate it to avoid
        // duplication.
        PerMethod() : rules(1) {}

        void internalAdd(std::string_view rule, RfOemBaseRule* ruleObject)
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

    void internalAddRuleObject(const std::string& rule,
                               RfOemBaseRule* ruleObject)
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
        for (std::unique_ptr<RfOemBaseRule>& rule : allRules)
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
        std::vector<RfOemBaseRule*> fragmentRules;
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

        Trie::FindResult found = perMethod.trie.find(url);
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

    FindRouteResponse findRoute(const Request& req) const
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

    void handleOemGet(const std::shared_ptr<Request>& req,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) const
    {
        BMCWEB_LOG_DEBUG("Checking OEM routes");
        FindRouteResponse foundRoute = findRoute(*req);
        std::vector<RfOemBaseRule*> fragments =
            std::move(foundRoute.route.fragmentRules);
        std::vector<std::string> params = std::move(foundRoute.route.params);
        if (!fragments.empty())
        {
            handleMultiFragmentGetRouting(req, fragments, params, asyncResp);
        }
        else
        {
            BMCWEB_LOG_DEBUG("No OEM routes found");
        }
    }

    void handleOemPatch(const std::shared_ptr<Request>& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, const nlohmann::json::object_t& payload)
    {
        BMCWEB_LOG_DEBUG("Checking OEM routes");
        FindRouteResponse foundRoute = findRoute(*req);
        std::vector<RfOemBaseRule*> fragments =
            std::move(foundRoute.route.fragmentRules);
        std::vector<std::string> params = std::move(foundRoute.route.params);
        if (!fragments.empty())
        {
            handleMultiFragmentPatchRouting(req, fragments, params, asyncResp, payload);
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
    std::vector<std::unique_ptr<RfOemBaseRule>> allRules;
};
} // namespace crow
