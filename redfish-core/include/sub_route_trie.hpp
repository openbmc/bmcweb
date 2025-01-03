// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "routing/trie.hpp"

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

template <typename ContainedType>
class SubRouteTrie
{
  public:
    struct SubRouteNode : public Trie<ContainedType>::Node
    {
        using ChildMap = Trie<ContainedType>::Node::ChildMap;
        ChildMap fragmentChildren;

        bool isSimpleNode() const
        {
            return Trie<ContainedType>::Node::isSimpleNode() &&
                   fragmentChildren.empty();
        }
    };

    SubRouteTrie() : nodes(1) {}

  private:
    void optimizeNode(SubRouteNode& node)
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
            typename SubRouteNode::ChildMap merged;
            for (const typename SubRouteNode::ChildMap::value_type& kv :
                 node.children)
            {
                SubRouteNode& child = nodes[kv.second];
                if (child.isSimpleNode())
                {
                    for (const typename SubRouteNode::ChildMap::value_type&
                             childKv : child.children)
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

        for (const typename SubRouteNode::ChildMap::value_type& kv :
             node.children)
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
                                const SubRouteNode& node) const
    {
        for (const typename SubRouteNode::ChildMap::value_type& kv :
             node.children)
        {
            const std::string& fragment = kv.first;
            const SubRouteNode& child = nodes[kv.second];
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
        std::vector<unsigned> fragmentRuleIndexes;
    };

  private:
    FindResult findHelper(const std::string_view reqUrl,
                          const SubRouteNode& node,
                          std::vector<std::string>& params) const
    {
        if (reqUrl.empty())
        {
            FindResult result = {node.ruleIndex, params, {}};
            for (const auto& [fragment, fragmentRuleIndex] :
                 node.fragmentChildren)
            {
                result.fragmentRuleIndexes.push_back(fragmentRuleIndex);
            }
            return result;
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
                if (ret.ruleIndex != 0U || !ret.fragmentRuleIndexes.empty())
                {
                    return ret;
                }
                params.pop_back();
            }
        }

        if (node.pathParamChild != 0U)
        {
            params.emplace_back(reqUrl);
            FindResult ret = findHelper("", nodes[node.pathParamChild], params);
            if (ret.ruleIndex != 0U || !ret.fragmentRuleIndexes.empty())
            {
                return ret;
            }
            params.pop_back();
        }

        for (const typename SubRouteNode::ChildMap::value_type& kv :
             node.children)
        {
            const std::string& fragment = kv.first;
            const SubRouteNode& child = nodes[kv.second];

            if (reqUrl.starts_with(fragment))
            {
                FindResult ret =
                    findHelper(reqUrl.substr(fragment.size()), child, params);
                if (ret.ruleIndex != 0U || !ret.fragmentRuleIndexes.empty())
                {
                    return ret;
                }
            }
        }

        return {0U, std::vector<std::string>(), {}};
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

        std::string_view fragment;
        // Check if the URL contains a fragment (#)
        size_t fragmentPos = urlIn.find('#');
        size_t queryPos = urlIn.find('?');
        if (fragmentPos != std::string::npos && queryPos == std::string::npos &&
            fragmentPos != urlIn.length() - 1)
        {
            fragment = urlIn.substr(fragmentPos + 1);
            url = urlIn.substr(0, fragmentPos);
        }

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
                    SubRouteNode& node = nodes[idx];
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
        SubRouteNode& node = nodes[idx];
        if (!fragment.empty())
        {
            node.fragmentChildren.emplace(fragment, ruleIndex);
            return;
        }
        if (node.ruleIndex != 0U)
        {
            BMCWEB_LOG_CRITICAL("handler already exists for \"{}\"", urlIn);
            throw std::runtime_error(
                std::format("handler already exists for \"{}\"", urlIn));
        }
        node.ruleIndex = ruleIndex;
    }

  private:
    void debugNodePrint(SubRouteNode& n, size_t level)
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
        for (const typename SubRouteNode::ChildMap::value_type& kv :
             n.fragmentChildren)
        {
            BMCWEB_LOG_DEBUG("{}#{}", spaces, kv.first);
        }
        for (const typename SubRouteNode::ChildMap::value_type& kv : n.children)
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
    const SubRouteNode& head() const
    {
        return nodes.front();
    }

    SubRouteNode& head()
    {
        return nodes.front();
    }

    unsigned newNode()
    {
        nodes.resize(nodes.size() + 1);
        return static_cast<unsigned>(nodes.size() - 1);
    }

    std::vector<SubRouteNode> nodes;
};

} // namespace crow
