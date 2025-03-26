// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#pragma once

#include "logging.hpp"

#include <boost/container/flat_map.hpp>
#include <boost/container/small_vector.hpp>

#include <cstddef>
#include <format>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace crow
{

template <typename ContainedType>
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
            typename Node::ChildMap merged;
            for (const typename Node::ChildMap::value_type& kv : node.children)
            {
                Node& child = nodes[kv.second];
                if (child.isSimpleNode())
                {
                    for (const typename Node::ChildMap::value_type& childKv :
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

        for (const typename Node::ChildMap::value_type& kv : node.children)
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
        for (const typename Node::ChildMap::value_type& kv : node.children)
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
        unsigned ruleIndex = 0;
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

        for (const typename Node::ChildMap::value_type& kv : node.children)
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
        for (const typename Node::ChildMap::value_type& kv : n.children)
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

    std::vector<Node> nodes{};
};
} // namespace crow
