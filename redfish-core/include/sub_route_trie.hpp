// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "logging.hpp"
#include "routing/trie.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace crow
{

struct SubRouteNode : public crow::Node
{
    using ChildMap = crow::Node::ChildMap;
    ChildMap fragmentChildren;

    bool isSimpleNode() const
    {
        return crow::Node::isSimpleNode() && fragmentChildren.empty();
    }
};

template <typename ContainedType>
class SubRouteTrie : public crow::Trie<ContainedType>
{
  public:
    struct FindResult
    {
        std::vector<std::string> params;
        std::vector<unsigned> fragmentRuleIndexes;
    };

  private:
    FindResult findHelper(const std::string_view reqUrl,
                          const ContainedType& node,
                          std::vector<std::string>& params) const
    {
        if (reqUrl.empty())
        {
            FindResult result = {params, {}};
            for (const auto& [fragment, fragmentRuleIndex] :
                 node.fragmentChildren)
            {
                result.fragmentRuleIndexes.push_back(fragmentRuleIndex);
            }
            return result;
        }

        if (node.stringParamChild != 0U)
        {
            size_t epos = reqUrl.find('/');
            if (epos == std::string_view::npos)
            {
                params.emplace_back(reqUrl);
                FindResult ret =
                    findHelper("", this->nodes[node.stringParamChild], params);
                if (!ret.fragmentRuleIndexes.empty())
                {
                    return ret;
                }
                params.pop_back();
            }
            else
            {
                params.emplace_back(reqUrl.substr(0, epos));
                FindResult ret =
                    findHelper(reqUrl.substr(epos),
                               this->nodes[node.stringParamChild], params);
                if (!ret.fragmentRuleIndexes.empty())
                {
                    return ret;
                }
                params.pop_back();
            }
        }

        if (node.pathParamChild != 0U)
        {
            params.emplace_back(reqUrl);
            FindResult ret =
                findHelper("", this->nodes[node.pathParamChild], params);
            if (!ret.fragmentRuleIndexes.empty())
            {
                return ret;
            }
            params.pop_back();
        }

        for (const typename ContainedType::ChildMap::value_type& kv :
             node.children)
        {
            const std::string& fragment = kv.first;
            const ContainedType& child = this->nodes[kv.second];

            if (reqUrl.starts_with(fragment))
            {
                FindResult ret =
                    findHelper(reqUrl.substr(fragment.size()), child, params);
                if (!ret.fragmentRuleIndexes.empty())
                {
                    return ret;
                }
            }
        }

        return {std::vector<std::string>(), {}};
    }

  public:
    FindResult find(const std::string_view reqUrl) const
    {
        std::vector<std::string> start;
        return findHelper(reqUrl, this->head(), start);
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

        if (fragment.empty())
        {
            BMCWEB_LOG_CRITICAL("empty fragment on rule \"{}\"", urlIn);
            throw std::runtime_error(
                std::format("empty fragment on rule \"{}\"", urlIn));
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
                    ContainedType& node = this->nodes[idx];
                    size_t* param = &node.stringParamChild;
                    if (str1 == "<path>")
                    {
                        param = &node.pathParamChild;
                    }
                    if (*param == 0U)
                    {
                        *param = this->newNode();
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
            if (!this->nodes[idx].children.contains(piece))
            {
                unsigned newNodeIdx = this->newNode();
                this->nodes[idx].children.emplace(piece, newNodeIdx);
            }
            idx = this->nodes[idx].children[piece];
            url.remove_prefix(1);
        }
        ContainedType& node = this->nodes[idx];
        if (node.fragmentChildren.find(fragment) != node.fragmentChildren.end())
        {
            BMCWEB_LOG_CRITICAL(
                R"(fragment handler already exists for "{}" fragment "{}")",
                urlIn, fragment);
            throw std::runtime_error(std::format(
                R"(handler already exists for url "{}" fragment "{}")", urlIn,
                fragment));
        }

        node.fragmentChildren.emplace(fragment, ruleIndex);
    }

  private:
    void debugNodePrint(ContainedType& n, size_t level)
    {
        std::string spaces(level, ' ');
        if (n.stringParamChild != 0U)
        {
            BMCWEB_LOG_DEBUG("{}<str>", spaces);
            debugNodePrint(this->nodes[n.stringParamChild], level + 5);
        }
        if (n.pathParamChild != 0U)
        {
            BMCWEB_LOG_DEBUG("{} <path>", spaces);
            debugNodePrint(this->nodes[n.pathParamChild], level + 6);
        }
        for (const typename ContainedType::ChildMap::value_type& kv :
             n.fragmentChildren)
        {
            BMCWEB_LOG_DEBUG("{}#{}", spaces, kv.first);
        }
        for (const typename ContainedType::ChildMap::value_type& kv :
             n.children)
        {
            BMCWEB_LOG_DEBUG("{}{}", spaces, kv.first);
            debugNodePrint(this->nodes[kv.second], level + kv.first.size());
        }
    }

  public:
    void debugPrint()
    {
        debugNodePrint(this->head(), 0U);
    }
};

} // namespace crow
