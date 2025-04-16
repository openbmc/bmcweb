// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "dbus_privileges.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "routing/baserule.hpp"
#include "routing/dynamicrule.hpp"
#include "routing/taggedrule.hpp"
#include "verb.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/small_vector.hpp>

#include <algorithm>
#include <array>
#include <cerrno>
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

class Trie
{
  public:
    struct Node
    {
        unsigned ruleIndex = 0U;

        size_t stringParamChild = 0U;
        size_t pathParamChild = 0U;

        using ChildMap = boost::container::flat_map<
            std::string, unsigned, std::less<>,
            boost::container::small_vector<std::pair<std::string, unsigned>,
                                           1>>;
        ChildMap children;

        bool isSimpleNode() const
        {
            return ruleIndex == 0 && stringParamChild == 0 &&
                   pathParamChild == 0;
        }
    };

    Trie() : nodes(1) {}

  private:
    void optimizeNode(Node& node)
    {
        if (node.stringParamChild != 0U)
        {
            optimizeNode(nodes[node.stringParamChild]);
        }
        if (node.pathParamChild != 0U)
        {
            optimizeNode(nodes[node.pathParamChild]);
        }

        if (node.children.empty())
        {
            return;
        }
        while (true)
        {
            bool didMerge = false;
            Node::ChildMap merged;
            for (const Node::ChildMap::value_type& kv : node.children)
            {
                Node& child = nodes[kv.second];
                if (child.isSimpleNode())
                {
                    for (const Node::ChildMap::value_type& childKv :
                         child.children)
                    {
                        merged[kv.first + childKv.first] = childKv.second;
                        didMerge = true;
                    }
                }
                else
                {
                    merged[kv.first] = kv.second;
                }
            }
            node.children = std::move(merged);
            if (!didMerge)
            {
                break;
            }
        }

        for (const Node::ChildMap::value_type& kv : node.children)
        {
            optimizeNode(nodes[kv.second]);
        }
    }

    void optimize()
    {
        optimizeNode(head());
    }

  public:
    void validate()
    {
        optimize();
    }

    void findRouteIndexesHelper(std::string_view reqUrl,
                                std::vector<unsigned>& routeIndexes,
                                const Node& node) const
    {
        for (const Node::ChildMap::value_type& kv : node.children)
        {
            const std::string& fragment = kv.first;
            const Node& child = nodes[kv.second];
            if (reqUrl.empty())
            {
                if (child.ruleIndex != 0 && fragment != "/")
                {
                    routeIndexes.push_back(child.ruleIndex);
                }
                findRouteIndexesHelper(reqUrl, routeIndexes, child);
            }
            else
            {
                if (reqUrl.starts_with(fragment))
                {
                    findRouteIndexesHelper(reqUrl.substr(fragment.size()),
                                           routeIndexes, child);
                }
            }
        }
    }

    void findRouteIndexes(const std::string& reqUrl,
                          std::vector<unsigned>& routeIndexes) const
    {
        findRouteIndexesHelper(reqUrl, routeIndexes, head());
    }

    struct FindResult
    {
        unsigned ruleIndex;
        std::vector<std::string> params;
    };

  private:
    FindResult findHelper(const std::string_view reqUrl, const Node& node,
                          std::vector<std::string>& params) const
    {
        if (reqUrl.empty())
        {
            return {node.ruleIndex, params};
        }

        if (node.stringParamChild != 0U)
        {
            size_t epos = 0;
            for (; epos < reqUrl.size(); epos++)
            {
                if (reqUrl[epos] == '/')
                {
                    break;
                }
            }

            if (epos != 0)
            {
                params.emplace_back(reqUrl.substr(0, epos));
                FindResult ret = findHelper(
                    reqUrl.substr(epos), nodes[node.stringParamChild], params);
                if (ret.ruleIndex != 0U)
                {
                    return {ret.ruleIndex, std::move(ret.params)};
                }
                params.pop_back();
            }
        }

        if (node.pathParamChild != 0U)
        {
            params.emplace_back(reqUrl);
            FindResult ret = findHelper("", nodes[node.pathParamChild], params);
            if (ret.ruleIndex != 0U)
            {
                return {ret.ruleIndex, std::move(ret.params)};
            }
            params.pop_back();
        }

        for (const Node::ChildMap::value_type& kv : node.children)
        {
            const std::string& fragment = kv.first;
            const Node& child = nodes[kv.second];

            if (reqUrl.starts_with(fragment))
            {
                FindResult ret =
                    findHelper(reqUrl.substr(fragment.size()), child, params);
                if (ret.ruleIndex != 0U)
                {
                    return {ret.ruleIndex, std::move(ret.params)};
                }
            }
        }

        return {0U, std::vector<std::string>()};
    }

  public:
    FindResult find(const std::string_view reqUrl) const
    {
        std::vector<std::string> start;
        return findHelper(reqUrl, head(), start);
    }

    void add(std::string_view urlIn, unsigned ruleIndex)
    {
        size_t idx = 0;

        std::string_view url = urlIn;

        while (!url.empty())
        {
            char c = url[0];
            if (c == '<')
            {
                bool found = false;
                for (const std::string_view str1 :
                     {"<str>", "<string>", "<path>"})
                {
                    if (!url.starts_with(str1))
                    {
                        continue;
                    }
                    found = true;
                    Node& node = nodes[idx];
                    size_t* param = &node.stringParamChild;
                    if (str1 == "<path>")
                    {
                        param = &node.pathParamChild;
                    }
                    if (*param == 0U)
                    {
                        *param = newNode();
                    }
                    idx = *param;

                    url.remove_prefix(str1.size());
                    break;
                }
                if (found)
                {
                    continue;
                }

                BMCWEB_LOG_CRITICAL("Can't find tag for {}", urlIn);
                return;
            }
            std::string piece(&c, 1);
            if (!nodes[idx].children.contains(piece))
            {
                unsigned newNodeIdx = newNode();
                nodes[idx].children.emplace(piece, newNodeIdx);
            }
            idx = nodes[idx].children[piece];
            url.remove_prefix(1);
        }
        Node& node = nodes[idx];
        if (node.ruleIndex != 0U)
        {
            BMCWEB_LOG_CRITICAL("handler already exists for \"{}\"", urlIn);
            throw std::runtime_error(
                std::format("handler already exists for \"{}\"", urlIn));
        }
        node.ruleIndex = ruleIndex;
    }

  private:
    void debugNodePrint(Node& n, size_t level)
    {
        std::string spaces(level, ' ');
        if (n.stringParamChild != 0U)
        {
            BMCWEB_LOG_DEBUG("{}<str>", spaces);
            debugNodePrint(nodes[n.stringParamChild], level + 5);
        }
        if (n.pathParamChild != 0U)
        {
            BMCWEB_LOG_DEBUG("{} <path>", spaces);
            debugNodePrint(nodes[n.pathParamChild], level + 6);
        }
        for (const Node::ChildMap::value_type& kv : n.children)
        {
            BMCWEB_LOG_DEBUG("{}{}", spaces, kv.first);
            debugNodePrint(nodes[kv.second], level + kv.first.size());
        }
    }

  public:
    void debugPrint()
    {
        debugNodePrint(head(), 0U);
    }

  private:
    const Node& head() const
    {
        return nodes.front();
    }

    Node& head()
    {
        return nodes.front();
    }

    unsigned newNode()
    {
        nodes.resize(nodes.size() + 1);
        return static_cast<unsigned>(nodes.size() - 1);
    }

    std::vector<Node> nodes;
};

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
        Trie trie;
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

        Trie::FindResult found = perMethod.trie.find(url);
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
    FindRouteResponse findRoute(const boost::urls::url& url,
                                boost::beast::http::verb bv) const
    {
        FindRouteResponse findRoute;

        // Check to see if this url exists at any verb
        for (size_t perMethodIndex = 0; perMethodIndex <= maxVerbIndex;
             perMethodIndex++)
        {
            // Make sure it's safe to deference the array at that index
            static_assert(
                maxVerbIndex < std::tuple_size_v<decltype(perMethods)>);
            FindRoute route = findRouteByPerMethod(url.encoded_path(),
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

        std::optional<HttpVerb> verb = httpVerbFromBoost(bv);
        if (!verb)
        {
            return findRoute;
        }
        size_t reqMethodIndex = static_cast<size_t>(*verb);
        if (reqMethodIndex >= perMethods.size())
        {
            return findRoute;
        }

        FindRoute route = findRouteByPerMethod(url.encoded_path(),
                                               perMethods[reqMethodIndex]);
        if (route.rule != nullptr)
        {
            findRoute.route = route;
        }

        return findRoute;
    }
    FindRouteResponse findRoute(const Request& request) const
    {
        return findRoute(request.url(), request.method());
    }

    template <typename Adaptor>
    void handleUpgrade(const std::shared_ptr<Request>& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       Adaptor&& adaptor)
    {
        PerMethod& perMethod = upgradeRoutes;
        Trie& trie = perMethod.trie;
        std::vector<BaseRule*>& rules = perMethod.rules;

        Trie::FindResult found = trie.find(req->url().encoded_path());
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
