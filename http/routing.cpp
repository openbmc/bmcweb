#include <boost/beast/ssl/ssl_stream.hpp>
#include "../redfish-core/include/privileges.hpp"
#include "routing_class_decl.hpp"
#include "http_response_class_decl.hpp"
#include "websocket_class_decl.hpp"
#include <type_traits>

namespace crow {

size_t BaseRule::getMethods()
{
    return methodsBitfield;
}

bool BaseRule::checkPrivileges(const redfish::Privileges& userPrivileges)
{
    // If there are no privileges assigned, assume no privileges
    // required
    if (privilegesSet.empty())
    {
        return true;
    }

    for (const redfish::Privileges& requiredPrivileges : privilegesSet)
    {
        if (userPrivileges.isSupersetOf(requiredPrivileges))
        {
            return true;
        }
    }
    return false;
}

void DynamicRule::validate()
{
    if (!erasedHandler)
    {
        throw std::runtime_error(nameStr + (!nameStr.empty() ? ": " : "") +
                                    "no handler for url " + rule);
    }
}

void DynamicRule::handle(const Request& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const RoutingParams& params)
{
    erasedHandler(req, asyncResp, params);
}

void WebSocketRule::handleUpgrade(const Request& req, Response&,
                       boost::asio::ip::tcp::socket&& adaptor)
{
    crow::websocket::HandleUpgrade(req, std::move(adaptor),
        openHandler, messageHandler, closeHandler, errorHandler);
}
#ifdef BMCWEB_ENABLE_SSL
void WebSocketRule::handleUpgrade(const Request& req, Response&,
                    boost::beast::ssl_stream<boost::asio::ip::tcp::socket>&&
                        adaptor)
{
    crow::websocket::HandleUpgradeSSL(req, std::move(adaptor),
        openHandler, messageHandler, closeHandler, errorHandler);
}
#endif

Trie::Trie() : nodes(1) {}

void Trie::optimizeNode(Node* node)
{
    for (size_t x : node->paramChildrens)
    {
        if (!x)
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
    for (const std::pair<std::string, unsigned>& kv : node->children)
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
        decltype(node->children) merged;
        for (const std::pair<std::string, unsigned>& kv : node->children)
        {
            Node* child = &nodes[kv.second];
            for (const std::pair<std::string, unsigned>& childKv :
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
        for (const std::pair<std::string, unsigned>& kv : node->children)
        {
            Node* child = &nodes[kv.second];
            optimizeNode(child);
        }
    }
}

void Trie::optimize()
{
    optimizeNode(head());
}

void Trie::validate()
{
    optimize();
}

void Trie::findRouteIndexes(const std::string& reqUrl,
                          std::vector<unsigned>& routeIndexes,
                          const Trie::Node* node, unsigned pos) const
{
    if (node == nullptr)
    {
        node = head();
    }
    for (const std::pair<std::string, unsigned>& kv : node->children)
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

std::pair<unsigned, RoutingParams>
        Trie::find(const std::string_view reqUrl, const Node* node,
             size_t pos, RoutingParams* params) const
{
    RoutingParams empty;
    if (params == nullptr)
    {
        params = &empty;
    }

    unsigned found{};
    RoutingParams matchParams;

    if (node == nullptr)
    {
        node = head();
    }
    if (pos == reqUrl.size())
    {
        return {node->ruleIndex, *params};
    }

    auto updateFound =
        [&found, &matchParams](std::pair<unsigned, RoutingParams>& ret) {
            if (ret.first && (!found || found > ret.first))
            {
                found = ret.first;
                matchParams = std::move(ret.second);
            }
        };

    if (node->paramChildrens[static_cast<size_t>(ParamType::INT)])
    {
        char c = reqUrl[pos];
        if ((c >= '0' && c <= '9') || c == '+' || c == '-')
        {
            char* eptr;
            errno = 0;
            long long int value =
                std::strtoll(reqUrl.data() + pos, &eptr, 10);
            if (errno != ERANGE && eptr != reqUrl.data() + pos)
            {
                params->intParams.push_back(value);
                std::pair<unsigned, RoutingParams> ret =
                    find(reqUrl,
                            &nodes[node->paramChildrens[static_cast<size_t>(
                                ParamType::INT)]],
                            static_cast<size_t>(eptr - reqUrl.data()), params);
                updateFound(ret);
                params->intParams.pop_back();
            }
        }
    }

    if (node->paramChildrens[static_cast<size_t>(ParamType::UINT)])
    {
        char c = reqUrl[pos];
        if ((c >= '0' && c <= '9') || c == '+')
        {
            char* eptr;
            errno = 0;
            unsigned long long int value =
                std::strtoull(reqUrl.data() + pos, &eptr, 10);
            if (errno != ERANGE && eptr != reqUrl.data() + pos)
            {
                params->uintParams.push_back(value);
                std::pair<unsigned, RoutingParams> ret =
                    find(reqUrl,
                            &nodes[node->paramChildrens[static_cast<size_t>(
                                ParamType::UINT)]],
                            static_cast<size_t>(eptr - reqUrl.data()), params);
                updateFound(ret);
                params->uintParams.pop_back();
            }
        }
    }

    if (node->paramChildrens[static_cast<size_t>(ParamType::DOUBLE)])
    {
        char c = reqUrl[pos];
        if ((c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.')
        {
            char* eptr;
            errno = 0;
            double value = std::strtod(reqUrl.data() + pos, &eptr);
            if (errno != ERANGE && eptr != reqUrl.data() + pos)
            {
                params->doubleParams.push_back(value);
                std::pair<unsigned, RoutingParams> ret =
                    find(reqUrl,
                            &nodes[node->paramChildrens[static_cast<size_t>(
                                ParamType::DOUBLE)]],
                            static_cast<size_t>(eptr - reqUrl.data()), params);
                updateFound(ret);
                params->doubleParams.pop_back();
            }
        }
    }

    if (node->paramChildrens[static_cast<size_t>(ParamType::STRING)])
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
            params->stringParams.emplace_back(
                reqUrl.substr(pos, epos - pos));
            std::pair<unsigned, RoutingParams> ret =
                find(reqUrl,
                        &nodes[node->paramChildrens[static_cast<size_t>(
                            ParamType::STRING)]],
                        epos, params);
            updateFound(ret);
            params->stringParams.pop_back();
        }
    }

    if (node->paramChildrens[static_cast<size_t>(ParamType::PATH)])
    {
        size_t epos = reqUrl.size();

        if (epos != pos)
        {
            params->stringParams.emplace_back(
                reqUrl.substr(pos, epos - pos));
            std::pair<unsigned, RoutingParams> ret =
                find(reqUrl,
                        &nodes[node->paramChildrens[static_cast<size_t>(
                            ParamType::PATH)]],
                        epos, params);
            updateFound(ret);
            params->stringParams.pop_back();
        }
    }

    for (const std::pair<std::string, unsigned>& kv : node->children)
    {
        const std::string& fragment = kv.first;
        const Node* child = &nodes[kv.second];

        if (reqUrl.compare(pos, fragment.size(), fragment) == 0)
        {
            std::pair<unsigned, RoutingParams> ret =
                find(reqUrl, child, pos + fragment.size(), params);
            updateFound(ret);
        }
    }

    return {found, matchParams};
}

void Trie::add(const std::string& url, unsigned ruleIndex)
{
    size_t idx = 0;

    for (unsigned i = 0; i < url.size(); i++)
    {
        char c = url[i];
        if (c == '<')
        {
            const static std::array<std::pair<ParamType, std::string>, 7>
                paramTraits = {{
                    {ParamType::INT, "<int>"},
                    {ParamType::UINT, "<uint>"},
                    {ParamType::DOUBLE, "<float>"},
                    {ParamType::DOUBLE, "<double>"},
                    {ParamType::STRING, "<str>"},
                    {ParamType::STRING, "<string>"},
                    {ParamType::PATH, "<path>"},
                }};

            for (const std::pair<ParamType, std::string>& x : paramTraits)
            {
                if (url.compare(i, x.second.size(), x.second) == 0)
                {
                    size_t index = static_cast<size_t>(x.first);
                    if (!nodes[idx].paramChildrens[index])
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
            if (!nodes[idx].children.count(piece))
            {
                unsigned newNodeIdx = newNode();
                nodes[idx].children.emplace(piece, newNodeIdx);
            }
            idx = nodes[idx].children[piece];
        }
    }
    if (nodes[idx].ruleIndex)
    {
        throw std::runtime_error("handler already exists for " + url);
    }
    nodes[idx].ruleIndex = ruleIndex;
}

void Trie::debugNodePrint(Trie::Node* n, size_t level)
{
    for (size_t i = 0; i < static_cast<size_t>(ParamType::MAX); i++)
    {
        if (n->paramChildrens[i])
        {
            BMCWEB_LOG_DEBUG << std::string(
                2U * level, ' ') /*<< "("<<n->paramChildrens[i]<<") "*/;
            switch (static_cast<ParamType>(i))
            {
                case ParamType::INT:
                    BMCWEB_LOG_DEBUG << "<int>";
                    break;
                case ParamType::UINT:
                    BMCWEB_LOG_DEBUG << "<uint>";
                    break;
                case ParamType::DOUBLE:
                    BMCWEB_LOG_DEBUG << "<float>";
                    break;
                case ParamType::STRING:
                    BMCWEB_LOG_DEBUG << "<str>";
                    break;
                case ParamType::PATH:
                    BMCWEB_LOG_DEBUG << "<path>";
                    break;
                case ParamType::MAX:
                    BMCWEB_LOG_DEBUG << "<ERROR>";
                    break;
            }

            debugNodePrint(&nodes[n->paramChildrens[i]], level + 1);
        }
    }
    for (const std::pair<std::string, unsigned>& kv : n->children)
    {
        BMCWEB_LOG_DEBUG
            << std::string(2U * level, ' ') /*<< "(" << kv.second << ") "*/
            << kv.first;
        debugNodePrint(&nodes[kv.second], level + 1);
    }
}

void Trie::debugPrint()
{
    debugNodePrint(head(), 0U);
}

const Trie::Node* Trie::head() const
{
    return &nodes.front();
}

Trie::Node* Trie::head()
{
    return &nodes.front();
}

unsigned Trie::newNode()
{
    nodes.resize(nodes.size() + 1);
    return static_cast<unsigned>(nodes.size() - 1);
}

bool Trie::Node::isSimpleNode() const
{
    return !ruleIndex && std::all_of(std::begin(paramChildrens),
                                        std::end(paramChildrens),
                                        [](size_t x) { return !x; });
}

/*
template <typename... Args>
template <typename Func>
typename std::enable_if<
    black_magic::CallHelper<Func, black_magic::S<Args...>>::value,
    void>::type
    TaggedRule<Args...>::operator()(Func&& f)
{
    static_assert(
        black_magic::CallHelper<Func, black_magic::S<Args...>>::value ||
            black_magic::CallHelper<
                Func, black_magic::S<crow::Request, Args...>>::value,
        "Handler type is mismatched with URL parameters");
    static_assert(
        !std::is_same<void, decltype(f(std::declval<Args>()...))>::value,
        "Handler function cannot have void return type; valid return "
        "types: "
        "string, int, crow::response, nlohmann::json");

    handler = [f = std::move(f)](
                  const Request&,
                  const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                  Args... args) { asyncResp->res.result(f(args...)); };
}

template <typename... Args>
template <typename Func>
    typename std::enable_if<
        !black_magic::CallHelper<Func, black_magic::S<Args...>>::value &&
            black_magic::CallHelper<
                Func, black_magic::S<crow::Request, Args...>>::value,
        void>::type
        TaggedRule<Args...>::operator()(Func&& f)
{
    static_assert(
        black_magic::CallHelper<Func, black_magic::S<Args...>>::value ||
            black_magic::CallHelper<
                Func, black_magic::S<crow::Request, Args...>>::value,
        "Handler type is mismatched with URL parameters");
    static_assert(
        !std::is_same<void, decltype(f(std::declval<crow::Request>(),
                                        std::declval<Args>()...))>::value,
        "Handler function cannot have void return type; valid return "
        "types: "
        "string, int, crow::response,nlohmann::json");

    handler = [f = std::move(f)](
                    const crow::Request& req,
                    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                    Args... args) { asyncResp->res.result(f(req, args...)); };
}

template <typename... Args>
template <typename Func>
typename std::enable_if<
    !black_magic::CallHelper<Func, black_magic::S<Args...>>::value &&
        !black_magic::CallHelper<
            Func, black_magic::S<crow::Request, Args...>>::value,
    void>::type
    TaggedRule<Args...>::operator()(Func&& f)
{
    static_assert(
        black_magic::CallHelper<Func, black_magic::S<Args...>>::value ||
            black_magic::CallHelper<
                Func, black_magic::S<crow::Request, Args...>>::value ||
            black_magic::CallHelper<
                Func, black_magic::S<crow::Request,
                                        std::shared_ptr<bmcweb::AsyncResp>&,
                                        Args...>>::value,
        "Handler type is mismatched with URL parameters");
    static_assert(
        std::is_same<
            void,
            decltype(f(std::declval<crow::Request>(),
                        std::declval<std::shared_ptr<bmcweb::AsyncResp>&>(),
                        std::declval<Args>()...))>::value,
        "Handler function with response argument should have void "
        "return "
        "type");

    handler = std::move(f);
}
*/

}