#pragma once

#include "async_resp.hpp"
#include "common.hpp"
#include "dbus_privileges.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "http_stream.hpp"
#include "logging.hpp"
#include "privileges.hpp"
#include "routing/baserule.hpp"
#include "routing/dynamicrule.hpp"
#include "routing/sserule.hpp"
#include "routing/taggedrule.hpp"
#include "routing/websocketrule.hpp"
#include "sessions.hpp"
#include "utility.hpp"
#include "utils/dbus_utils.hpp"
#include "verb.hpp"
#include "websocket.hpp"

#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <optional>
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
        unsigned ruleIndex{};
        std::array<size_t, static_cast<size_t>(ParamType::MAX)>
            paramChildrens{};
        using ChildMap = boost::container::flat_map<
            std::string, unsigned, std::less<>,
            std::vector<std::pair<std::string, unsigned>>>;
        ChildMap children;

        bool isSimpleNode() const
        {
            return ruleIndex == 0 &&
                   std::all_of(std::begin(paramChildrens),
                               std::end(paramChildrens),
                               [](size_t x) { return x == 0U; });
        }
    };

    Trie() : nodes(1) {}

  private:
    void optimizeNode(Node* node)
    {
        for (size_t x : node->paramChildrens)
        {
            if (x == 0U)
            {
                continue;
            }
            Node* child = &nodes[x];
            optimizeNode(child);
        }
        if (node->children.empty())
        {
            return;
        }
        bool mergeWithChild = true;
        for (const Node::ChildMap::value_type& kv : node->children)
        {
            Node* child = &nodes[kv.second];
            if (!child->isSimpleNode())
            {
                mergeWithChild = false;
                break;
            }
        }
        if (mergeWithChild)
        {
            Node::ChildMap merged;
            for (const Node::ChildMap::value_type& kv : node->children)
            {
                Node* child = &nodes[kv.second];
                for (const Node::ChildMap::value_type& childKv :
                     child->children)
                {
                    merged[kv.first + childKv.first] = childKv.second;
                }
            }
            node->children = std::move(merged);
            optimizeNode(node);
        }
        else
        {
            for (const Node::ChildMap::value_type& kv : node->children)
            {
                Node* child = &nodes[kv.second];
                optimizeNode(child);
            }
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

    void findRouteIndexes(const std::string& reqUrl,
                          std::vector<unsigned>& routeIndexes,
                          const Node* node = nullptr, unsigned pos = 0) const
    {
        if (node == nullptr)
        {
            node = head();
        }
        for (const Node::ChildMap::value_type& kv : node->children)
        {
            const std::string& fragment = kv.first;
            const Node* child = &nodes[kv.second];
            if (pos >= reqUrl.size())
            {
                if (child->ruleIndex != 0 && fragment != "/")
                {
                    routeIndexes.push_back(child->ruleIndex);
                }
                findRouteIndexes(reqUrl, routeIndexes, child,
                                 static_cast<unsigned>(pos + fragment.size()));
            }
            else
            {
                if (reqUrl.compare(pos, fragment.size(), fragment) == 0)
                {
                    findRouteIndexes(
                        reqUrl, routeIndexes, child,
                        static_cast<unsigned>(pos + fragment.size()));
                }
            }
        }
    }

    std::pair<unsigned, std::vector<std::string>>
        find(const std::string_view reqUrl, const Node* node = nullptr,
             size_t pos = 0, std::vector<std::string>* params = nullptr) const
    {
        std::vector<std::string> empty;
        if (params == nullptr)
        {
            params = &empty;
        }

        unsigned found{};
        std::vector<std::string> matchParams;

        if (node == nullptr)
        {
            node = head();
        }
        if (pos == reqUrl.size())
        {
            return {node->ruleIndex, *params};
        }

        auto updateFound =
            [&found,
             &matchParams](std::pair<unsigned, std::vector<std::string>>& ret) {
            if (ret.first != 0U && (found == 0U || found > ret.first))
            {
                found = ret.first;
                matchParams = std::move(ret.second);
            }
        };

        if (node->paramChildrens[static_cast<size_t>(ParamType::STRING)] != 0U)
        {
            size_t epos = pos;
            for (; epos < reqUrl.size(); epos++)
            {
                if (reqUrl[epos] == '/')
                {
                    break;
                }
            }

            if (epos != pos)
            {
                params->emplace_back(reqUrl.substr(pos, epos - pos));
                std::pair<unsigned, std::vector<std::string>> ret =
                    find(reqUrl,
                         &nodes[node->paramChildrens[static_cast<size_t>(
                             ParamType::STRING)]],
                         epos, params);
                updateFound(ret);
                params->pop_back();
            }
        }

        if (node->paramChildrens[static_cast<size_t>(ParamType::PATH)] != 0U)
        {
            size_t epos = reqUrl.size();

            if (epos != pos)
            {
                params->emplace_back(reqUrl.substr(pos, epos - pos));
                std::pair<unsigned, std::vector<std::string>> ret =
                    find(reqUrl,
                         &nodes[node->paramChildrens[static_cast<size_t>(
                             ParamType::PATH)]],
                         epos, params);
                updateFound(ret);
                params->pop_back();
            }
        }

        for (const Node::ChildMap::value_type& kv : node->children)
        {
            const std::string& fragment = kv.first;
            const Node* child = &nodes[kv.second];

            if (reqUrl.compare(pos, fragment.size(), fragment) == 0)
            {
                std::pair<unsigned, std::vector<std::string>> ret =
                    find(reqUrl, child, pos + fragment.size(), params);
                updateFound(ret);
            }
        }

        return {found, matchParams};
    }

    void add(const std::string& url, unsigned ruleIndex)
    {
        size_t idx = 0;

        for (unsigned i = 0; i < url.size(); i++)
        {
            char c = url[i];
            if (c == '<')
            {
                constexpr static std::array<
                    std::pair<ParamType, std::string_view>, 3>
                    paramTraits = {{
                        {ParamType::STRING, "<str>"},
                        {ParamType::STRING, "<string>"},
                        {ParamType::PATH, "<path>"},
                    }};

                for (const std::pair<ParamType, std::string_view>& x :
                     paramTraits)
                {
                    if (url.compare(i, x.second.size(), x.second) == 0)
                    {
                        size_t index = static_cast<size_t>(x.first);
                        if (nodes[idx].paramChildrens[index] == 0U)
                        {
                            unsigned newNodeIdx = newNode();
                            nodes[idx].paramChildrens[index] = newNodeIdx;
                        }
                        idx = nodes[idx].paramChildrens[index];
                        i += static_cast<unsigned>(x.second.size());
                        break;
                    }
                }

                i--;
            }
            else
            {
                std::string piece(&c, 1);
                if (nodes[idx].children.count(piece) == 0U)
                {
                    unsigned newNodeIdx = newNode();
                    nodes[idx].children.emplace(piece, newNodeIdx);
                }
                idx = nodes[idx].children[piece];
            }
        }
        if (nodes[idx].ruleIndex != 0U)
        {
            throw std::runtime_error("handler already exists for " + url);
        }
        nodes[idx].ruleIndex = ruleIndex;
    }

  private:
    void debugNodePrint(Node* n, size_t level)
    {
        std::string indent(2U * level, ' ');
        for (size_t i = 0; i < static_cast<size_t>(ParamType::MAX); i++)
        {
            if (n->paramChildrens[i] != 0U)
            {
                switch (static_cast<ParamType>(i))
                {
                    case ParamType::STRING:
                        BMCWEB_LOG_DEBUG("{}({}) <str>", indent,
                                         n->paramChildrens[i]);
                        break;
                    case ParamType::PATH:
                        BMCWEB_LOG_DEBUG("{}({}) <path>", indent,
                                         n->paramChildrens[i]);
                        BMCWEB_LOG_DEBUG("<path>");
                        break;
                    default:
                        BMCWEB_LOG_DEBUG("{}<ERROR>", indent);
                        break;
                }

                debugNodePrint(&nodes[n->paramChildrens[i]], level + 1);
            }
        }
        for (const Node::ChildMap::value_type& kv : n->children)
        {
            BMCWEB_LOG_DEBUG("{}({}{}) ", indent, kv.second, kv.first);
            debugNodePrint(&nodes[kv.second], level + 1);
        }
    }

  public:
    void debugPrint()
    {
        debugNodePrint(head(), 0U);
    }

  private:
    const Node* head() const
    {
        return &nodes.front();
    }

    Node* head()
    {
        return &nodes.front();
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

    template <uint64_t N>
    auto& newRuleTagged(const std::string& rule)
    {
        constexpr size_t numArgs = utility::numArgsFromTag(N);
        if constexpr (numArgs == 0)
        {
            using RuleT = TaggedRule<>;
            std::unique_ptr<RuleT> ruleObject = std::make_unique<RuleT>(rule);
            RuleT* ptr = ruleObject.get();
            allRules.emplace_back(std::move(ruleObject));
            return *ptr;
        }
        else if constexpr (numArgs == 1)
        {
            using RuleT = TaggedRule<std::string>;
            std::unique_ptr<RuleT> ruleObject = std::make_unique<RuleT>(rule);
            RuleT* ptr = ruleObject.get();
            allRules.emplace_back(std::move(ruleObject));
            return *ptr;
        }
        else if constexpr (numArgs == 2)
        {
            using RuleT = TaggedRule<std::string, std::string>;
            std::unique_ptr<RuleT> ruleObject = std::make_unique<RuleT>(rule);
            RuleT* ptr = ruleObject.get();
            allRules.emplace_back(std::move(ruleObject));
            return *ptr;
        }
        else if constexpr (numArgs == 3)
        {
            using RuleT = TaggedRule<std::string, std::string, std::string>;
            std::unique_ptr<RuleT> ruleObject = std::make_unique<RuleT>(rule);
            RuleT* ptr = ruleObject.get();
            allRules.emplace_back(std::move(ruleObject));
            return *ptr;
        }
        else if constexpr (numArgs == 4)
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
        static_assert(numArgs < 5, "Max number of args supported is 5");
    }

    void internalAddRuleObject(const std::string& rule, BaseRule* ruleObject)
    {
        if (ruleObject == nullptr)
        {
            return;
        }
        for (size_t method = 0, methodBit = 1; method <= methodNotAllowedIndex;
             method++, methodBit <<= 1)
        {
            if ((ruleObject->methodsBitfield & methodBit) > 0U)
            {
                perMethods[method].rules.emplace_back(ruleObject);
                perMethods[method].trie.add(
                    rule, static_cast<unsigned>(
                              perMethods[method].rules.size() - 1U));
                // directory case:
                //   request to `/about' url matches `/about/' rule
                if (rule.size() > 2 && rule.back() == '/')
                {
                    perMethods[method].trie.add(
                        rule.substr(0, rule.size() - 1),
                        static_cast<unsigned>(perMethods[method].rules.size() -
                                              1));
                }
            }
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

    FindRoute findRouteByIndex(std::string_view url, size_t index) const
    {
        FindRoute route;
        if (index >= perMethods.size())
        {
            BMCWEB_LOG_CRITICAL("Bad index???");
            return route;
        }
        const PerMethod& perMethod = perMethods[index];
        std::pair<unsigned, std::vector<std::string>> found =
            perMethod.trie.find(url);
        if (found.first >= perMethod.rules.size())
        {
            throw std::runtime_error("Trie internal structure corrupted!");
        }
        // Found a 404 route, switch that in
        if (found.first != 0U)
        {
            route.rule = perMethod.rules[found.first];
            route.params = std::move(found.second);
        }
        return route;
    }

    FindRouteResponse findRoute(Request& req) const
    {
        FindRouteResponse findRoute;

        std::optional<HttpVerb> verb = httpVerbFromBoost(req.method());
        if (!verb)
        {
            return findRoute;
        }
        size_t reqMethodIndex = static_cast<size_t>(*verb);
        // Check to see if this url exists at any verb
        for (size_t perMethodIndex = 0; perMethodIndex <= maxVerbIndex;
             perMethodIndex++)
        {
            // Make sure it's safe to deference the array at that index
            static_assert(maxVerbIndex <
                          std::tuple_size_v<decltype(perMethods)>);
            FindRoute route = findRouteByIndex(req.url().encoded_path(),
                                               perMethodIndex);
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
            if (perMethodIndex == reqMethodIndex)
            {
                findRoute.route = route;
            }
        }
        return findRoute;
    }

    template <typename Adaptor>
    void handleUpgrade(Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       Adaptor&& adaptor)
    {
        std::optional<HttpVerb> verb = httpVerbFromBoost(req.method());
        if (!verb || static_cast<size_t>(*verb) >= perMethods.size())
        {
            asyncResp->res.result(boost::beast::http::status::not_found);
            return;
        }
        PerMethod& perMethod = perMethods[static_cast<size_t>(*verb)];
        Trie& trie = perMethod.trie;
        std::vector<BaseRule*>& rules = perMethod.rules;

        const std::pair<unsigned, std::vector<std::string>>& found =
            trie.find(req.url().encoded_path());
        unsigned ruleIndex = found.first;
        if (ruleIndex == 0U)
        {
            BMCWEB_LOG_DEBUG("Cannot match rules {}", req.url().encoded_path());
            asyncResp->res.result(boost::beast::http::status::not_found);
            return;
        }

        if (ruleIndex >= rules.size())
        {
            throw std::runtime_error("Trie internal structure corrupted!");
        }

        BaseRule& rule = *rules[ruleIndex];
        size_t methods = rule.getMethods();
        if ((methods & (1U << static_cast<size_t>(*verb))) == 0)
        {
            BMCWEB_LOG_DEBUG(
                "Rule found but method mismatch: {} with {}({}) / {}",
                req.url().encoded_path(), req.methodString(),
                static_cast<uint32_t>(*verb), methods);
            asyncResp->res.result(boost::beast::http::status::not_found);
            return;
        }

        BMCWEB_LOG_DEBUG("Matched rule (upgrade) '{}' {} / {}", rule.rule,
                         static_cast<uint32_t>(*verb), methods);

        // TODO(ed) This should be able to use std::bind_front, but it doesn't
        // appear to work with the std::move on adaptor.
        validatePrivilege(
            req, asyncResp, rule,
            [&rule, asyncResp, adaptor = std::forward<Adaptor>(adaptor)](
                Request& thisReq) mutable {
            rule.handleUpgrade(thisReq, asyncResp, std::move(adaptor));
        });
    }

    void handle(Request& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
    {
        std::optional<HttpVerb> verb = httpVerbFromBoost(req.method());
        if (!verb || static_cast<size_t>(*verb) >= perMethods.size())
        {
            asyncResp->res.result(boost::beast::http::status::not_found);
            return;
        }

        FindRouteResponse foundRoute = findRoute(req);

        if (foundRoute.route.rule == nullptr)
        {
            // Couldn't find a normal route with any verb, try looking for a 404
            // route
            if (foundRoute.allowHeader.empty())
            {
                foundRoute.route = findRouteByIndex(req.url().encoded_path(),
                                                    notFoundIndex);
            }
            else
            {
                // See if we have a method not allowed (405) handler
                foundRoute.route = findRouteByIndex(req.url().encoded_path(),
                                                    methodNotAllowedIndex);
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
                         static_cast<uint32_t>(*verb), rule.getMethods());

        if (req.session == nullptr)
        {
            rule.handle(req, asyncResp, params);
            return;
        }
        validatePrivilege(req, asyncResp, rule,
                          [&rule, asyncResp, params](Request& thisReq) mutable {
            rule.handle(thisReq, asyncResp, params);
        });
    }

    void debugPrint()
    {
        for (size_t i = 0; i < perMethods.size(); i++)
        {
            BMCWEB_LOG_DEBUG("{}",
                             boost::beast::http::to_string(
                                 static_cast<boost::beast::http::verb>(i)));
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
    struct PerMethod
    {
        std::vector<BaseRule*> rules;
        Trie trie;
        // rule index 0 has special meaning; preallocate it to avoid
        // duplication.
        PerMethod() : rules(1) {}
    };

    std::array<PerMethod, methodNotAllowedIndex + 1> perMethods;
    std::vector<std::unique_ptr<BaseRule>> allRules;
};
} // namespace crow
