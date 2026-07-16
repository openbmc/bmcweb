// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "dbus_privileges.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "privileges.hpp"
#include "routing/baserule.hpp"
#include "routing/dynamicrule.hpp"
#include "routing/taggedrule.hpp"
#include "routing/trie.hpp"
#include "verb.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace crow
{

class Router
{
  public:
    Router() = default;

    DynamicRule& newRuleDynamic(const std::string& rule)
    {
        std::unique_ptr<DynamicRule> ruleObject =
            std::make_unique<DynamicRule>(rule);
        DynamicRule* ptr = ruleObject.get();
        allRules.emplace_back(std::move(ruleObject));

        return *ptr;
    }

    template <uint64_t NumArgs>
    auto& newRuleTagged(const std::string& rule)
    {
        if constexpr (NumArgs == 0)
        {
            using RuleT = TaggedRule<>;
            std::unique_ptr<RuleT> ruleObject = std::make_unique<RuleT>(rule);
            RuleT* ptr = ruleObject.get();
            allRules.emplace_back(std::move(ruleObject));
            return *ptr;
        }
        else if constexpr (NumArgs == 1)
        {
            using RuleT = TaggedRule<std::string>;
            std::unique_ptr<RuleT> ruleObject = std::make_unique<RuleT>(rule);
            RuleT* ptr = ruleObject.get();
            allRules.emplace_back(std::move(ruleObject));
            return *ptr;
        }
        else if constexpr (NumArgs == 2)
        {
            using RuleT = TaggedRule<std::string, std::string>;
            std::unique_ptr<RuleT> ruleObject = std::make_unique<RuleT>(rule);
            RuleT* ptr = ruleObject.get();
            allRules.emplace_back(std::move(ruleObject));
            return *ptr;
        }
        else if constexpr (NumArgs == 3)
        {
            using RuleT = TaggedRule<std::string, std::string, std::string>;
            std::unique_ptr<RuleT> ruleObject = std::make_unique<RuleT>(rule);
            RuleT* ptr = ruleObject.get();
            allRules.emplace_back(std::move(ruleObject));
            return *ptr;
        }
        else if constexpr (NumArgs == 4)
        {
            using RuleT =
                TaggedRule<std::string, std::string, std::string, std::string>;
            std::unique_ptr<RuleT> ruleObject = std::make_unique<RuleT>(rule);
            RuleT* ptr = ruleObject.get();
            allRules.emplace_back(std::move(ruleObject));
            return *ptr;
        }
        else
        {
            using RuleT = TaggedRule<std::string, std::string, std::string,
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
        std::vector<BaseRule*> rules;
        Trie<crow::Node> trie;
        // rule index 0 has special meaning; preallocate it to avoid
        // duplication.
        PerMethod() : rules(1) {}

        void internalAdd(std::string_view rule, BaseRule* ruleObject)
        {
            rules.emplace_back(ruleObject);
            trie.add(rule, static_cast<unsigned>(rules.size() - 1U));
            // directory case:
            //   request to `/about' url matches `/about/' rule
            if (rule.size() > 2 && rule.back() == '/')
            {
                trie.add(rule.substr(0, rule.size() - 1),
                         static_cast<unsigned>(rules.size() - 1));
            }
        }
    };

    void internalAddRuleObject(const std::string& rule, BaseRule* ruleObject)
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

        if (ruleObject->isNotFound)
        {
            notFoundRoutes.internalAdd(rule, ruleObject);
        }

        if (ruleObject->isMethodNotAllowed)
        {
            methodNotAllowedRoutes.internalAdd(rule, ruleObject);
        }

        if (ruleObject->isUpgrade)
        {
            upgradeRoutes.internalAdd(rule, ruleObject);
        }
    }

    void validate()
    {
        for (std::unique_ptr<BaseRule>& rule : allRules)
        {
            if (rule)
            {
                std::unique_ptr<BaseRule> upgraded = rule->upgrade();
                if (upgraded)
                {
                    rule = std::move(upgraded);
                }
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
        BaseRule* rule = nullptr;
        std::vector<std::string> params;
    };

    struct FindRouteResponse
    {
        std::string allowHeader;
        FindRoute route;
    };

    static FindRoute findRouteByPerMethod(std::string_view url,
                                          const PerMethod& perMethod)
    {
        FindRoute route;

        Trie<crow::Node>::FindResult found = perMethod.trie.find(url);
        if (found.ruleIndex >= perMethod.rules.size())
        {
            throw std::runtime_error("Trie internal structure corrupted!");
        }
        // Found a 404 route, switch that in
        if (found.ruleIndex != 0U)
        {
            route.rule = perMethod.rules[found.ruleIndex];
            route.params = std::move(found.params);
        }
        return route;
    }

    FindRouteResponse findRoute(const Request& req) const
    {
        FindRouteResponse findRoute;

        // Check to see if this url exists at any verb
        for (size_t perMethodIndex = 0; perMethodIndex <= maxVerbIndex;
             perMethodIndex++)
        {
            // Make sure it's safe to deference the array at that index
            static_assert(
                maxVerbIndex < std::tuple_size_v<decltype(perMethods)>);
            FindRoute route = findRouteByPerMethod(req.url().encoded_path(),
                                                   perMethods[perMethodIndex]);
            if (route.rule == nullptr)
            {
                continue;
            }
            if (!findRoute.allowHeader.empty())
            {
                findRoute.allowHeader += ", ";
            }
            HttpVerb thisVerb = static_cast<HttpVerb>(perMethodIndex);
            findRoute.allowHeader += httpVerbToString(thisVerb);
        }

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
        if (route.rule != nullptr)
        {
            findRoute.route = route;
        }

        return findRoute;
    }

    template <typename Adaptor>
    void handleUpgrade(const std::shared_ptr<Request>& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       Adaptor&& adaptor)
    {
        PerMethod& perMethod = upgradeRoutes;
        Trie<crow::Node>& trie = perMethod.trie;
        std::vector<BaseRule*>& rules = perMethod.rules;

        Trie<crow::Node>::FindResult found =
            trie.find(req->url().encoded_path());
        unsigned ruleIndex = found.ruleIndex;
        if (ruleIndex == 0U)
        {
            BMCWEB_LOG_DEBUG("Cannot match rules {}",
                             req->url().encoded_path());
            asyncResp->res.result(boost::beast::http::status::not_found);
            return;
        }

        if (ruleIndex >= rules.size())
        {
            throw std::runtime_error("Trie internal structure corrupted!");
        }

        BaseRule& rule = *rules[ruleIndex];

        BMCWEB_LOG_DEBUG("Matched rule (upgrade) '{}'", rule.rule);

        // TODO(ed) This should be able to use std::bind_front, but it doesn't
        // appear to work with the std::move on adaptor.
        validatePrivilege(
            req, asyncResp, rule,
            [req, &rule, asyncResp,
             adaptor = std::forward<Adaptor>(adaptor)]() mutable {
                rule.handleUpgrade(*req, asyncResp, std::move(adaptor));
            });
    }

    void handle(const std::shared_ptr<Request>& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
    {
        FindRouteResponse foundRoute = findRoute(*req);

        if (foundRoute.route.rule == nullptr && !foundRoute.allowHeader.empty())
        {
            // A different verb is registered here, but per DSP0266 7.5.3,
            // 405 is only correct if the resource exists; otherwise it's
            // 404.  Probe the GET route (safe: no side effects) to decide,
            // falling back to the nearest ancestor's GET if this exact URL
            // has none of its own.
            FindRoute getRoute =
                findGetRouteForNearestAncestor(req->url().encoded_path());
            if (getRoute.rule != nullptr &&
                callerMayProbeExistence(req, *getRoute.rule))
            {
                probeExistenceThenFinish(req, asyncResp, *getRoute.rule,
                                         std::move(getRoute.params),
                                         foundRoute.allowHeader);
                return;
            }
        }

        finishHandle(req, asyncResp, foundRoute);
    }

  private:
    // Only probe existence for callers who could legitimately GET this
    // resource themselves.  Some GET handlers (e.g. ManagerAccount) return
    // 403 for reasons unrelated to existence, which would otherwise leak
    // existence via 404-vs-405 to unprivileged callers (DSP0266 13.2
    // enumeration risk).  Callers who don't qualify just get the plain
    // 404/405 catch-all, as before this probe existed.
    static bool callerMayProbeExistence(const std::shared_ptr<Request>& req,
                                        BaseRule& getRule)
    {
        if (req->session == nullptr)
        {
            return false;
        }
        redfish::Privileges userPrivileges =
            redfish::getUserPrivileges(*req->session);
        if (req->session->isConfigureSelfOnly)
        {
            userPrivileges = userPrivileges.intersection(
                redfish::Privileges{"ConfigureSelf"});
        }
        return getRule.checkPrivileges(userPrivileges);
    }

    // Finds the GET rule for this exact URL, or walks up the path to the
    // nearest ancestor that has one (e.g. for POST-only Action targets) —
    // a sub-resource can only exist if its parent does.
    FindRoute findGetRouteForNearestAncestor(std::string_view path) const
    {
        while (true)
        {
            FindRoute route = findRouteByPerMethod(
                path, perMethods[static_cast<size_t>(HttpVerb::Get)]);
            if (route.rule != nullptr)
            {
                return route;
            }
            if (path.size() <= 1)
            {
                return {};
            }
            // Drop the trailing slash (if any), then the last path segment,
            // leaving the parent's trailing slash intact so it matches
            // routes registered as ".../parent/".
            std::string_view trimmed = path;
            if (trimmed.back() == '/')
            {
                trimmed.remove_suffix(1);
            }
            size_t slash = trimmed.find_last_of('/');
            if (slash == std::string_view::npos)
            {
                return {};
            }
            path = path.substr(0, slash + 1);
        }
    }

    // Completes routing with the chosen catch-all; shared by the direct
    // dispatch path and the existence-probe callback below.
    void finishHandle(const std::shared_ptr<Request>& req,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      FindRouteResponse& foundRoute)
    {
        if (foundRoute.route.rule == nullptr)
        {
            // Couldn't find a normal route with any verb, try looking for a 404
            // route
            if (foundRoute.allowHeader.empty())
            {
                foundRoute.route = findRouteByPerMethod(
                    req->url().encoded_path(), notFoundRoutes);
            }
            else
            {
                // See if we have a method not allowed (405) handler
                foundRoute.route = findRouteByPerMethod(
                    req->url().encoded_path(), methodNotAllowedRoutes);
            }
        }

        // Fill in the allow header if it's valid
        if (!foundRoute.allowHeader.empty())
        {
            asyncResp->res.addHeader(boost::beast::http::field::allow,
                                     foundRoute.allowHeader);
        }

        // If we couldn't find a real route or a 404 route, return a generic
        // response
        if (foundRoute.route.rule == nullptr)
        {
            if (foundRoute.allowHeader.empty())
            {
                asyncResp->res.result(boost::beast::http::status::not_found);
            }
            else
            {
                asyncResp->res.result(
                    boost::beast::http::status::method_not_allowed);
            }
            return;
        }

        BaseRule& rule = *foundRoute.route.rule;
        std::vector<std::string> params = std::move(foundRoute.route.params);

        BMCWEB_LOG_DEBUG("Matched rule '{}' {} / {}", rule.rule,
                         req->methodString(), rule.getMethods());

        if (req->session == nullptr)
        {
            rule.handle(*req, asyncResp, params);
            return;
        }
        validatePrivilege(
            req, asyncResp, rule,
            [req, asyncResp, &rule, params = std::move(params)]() {
                rule.handle(*req, asyncResp, params);
            });
    }

    // Probes the GET handler to disambiguate 404 vs 405.  Only its status
    // code is used, never the body, to pick the right catch-all.  Only
    // called for callers that already passed callerMayProbeExistence().
    void probeExistenceThenFinish(
        const std::shared_ptr<Request>& req,
        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, BaseRule& getRule,
        std::vector<std::string> getParams, std::string allowHeader)
    {
        auto probeResp = std::make_shared<bmcweb::AsyncResp>();
        probeResp->res.setCompleteRequestHandler(
            [this, req, asyncResp, allowHeader = std::move(allowHeader)](
                crow::Response& probeResult) {
                FindRouteResponse followUp;
                followUp.allowHeader =
                    probeResult.result() ==
                            boost::beast::http::status::not_found
                        ? ""
                        : allowHeader;
                finishHandle(req, asyncResp, followUp);
            });
        // Bypasses the GET handler's own privilege check; safe since the
        // caller already passed callerMayProbeExistence() above, and only
        // the status code (never the body) is used.
        getRule.handle(*req, probeResp, getParams);
    }

  public:
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

    PerMethod notFoundRoutes;
    PerMethod upgradeRoutes;
    PerMethod methodNotAllowedRoutes;

    std::vector<std::unique_ptr<BaseRule>> allRules;
};
} // namespace crow
