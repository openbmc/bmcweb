#pragma once

#include "boost/container/flat_map.hpp"

#include <boost/lexical_cast.hpp>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

#include "crow/common.h"
#include "crow/http_request.h"
#include "crow/http_response.h"
#include "crow/logging.h"
#include "crow/utility.h"
#include "crow/websocket.h"

namespace crow
{
class BaseRule
{
  public:
    BaseRule(std::string rule) : rule(std::move(rule))
    {
    }

    virtual ~BaseRule()
    {
    }

    virtual void validate() = 0;
    std::unique_ptr<BaseRule> upgrade()
    {
        if (ruleToUpgrade)
            return std::move(ruleToUpgrade);
        return {};
    }

    virtual void handle(const Request&, Response&, const RoutingParams&) = 0;
    virtual void handleUpgrade(const Request&, Response& res,
                               boost::asio::ip::tcp::socket&&)
    {
        res = Response(boost::beast::http::status::not_found);
        res.end();
    }
#ifdef BMCWEB_ENABLE_SSL
    virtual void
        handleUpgrade(const Request&, Response& res,
                      boost::beast::ssl_stream<boost::asio::ip::tcp::socket>&&)
    {
        res = Response(boost::beast::http::status::not_found);
        res.end();
    }
#endif

    uint32_t getMethods()
    {
        return methodsBitfield;
    }

  protected:
    uint32_t methodsBitfield{1 << (int)boost::beast::http::verb::get};

    std::string rule;
    std::string nameStr;

    std::unique_ptr<BaseRule> ruleToUpgrade;

    friend class Router;
    template <typename T> friend struct RuleParameterTraits;
};

namespace detail
{
namespace routing_handler_call_helper
{
template <typename T, int Pos> struct CallPair
{
    using type = T;
    static const int pos = Pos;
};

template <typename H1> struct CallParams
{
    H1& handler;
    const RoutingParams& params;
    const Request& req;
    Response& res;
};

template <typename F, int NInt, int NUint, int NDouble, int NString,
          typename S1, typename S2>
struct Call
{
};

template <typename F, int NInt, int NUint, int NDouble, int NString,
          typename... Args1, typename... Args2>
struct Call<F, NInt, NUint, NDouble, NString, black_magic::S<int64_t, Args1...>,
            black_magic::S<Args2...>>
{
    void operator()(F cparams)
    {
        using pushed = typename black_magic::S<Args2...>::template push_back<
            CallPair<int64_t, NInt>>;
        Call<F, NInt + 1, NUint, NDouble, NString, black_magic::S<Args1...>,
             pushed>()(cparams);
    }
};

template <typename F, int NInt, int NUint, int NDouble, int NString,
          typename... Args1, typename... Args2>
struct Call<F, NInt, NUint, NDouble, NString,
            black_magic::S<uint64_t, Args1...>, black_magic::S<Args2...>>
{
    void operator()(F cparams)
    {
        using pushed = typename black_magic::S<Args2...>::template push_back<
            CallPair<uint64_t, NUint>>;
        Call<F, NInt, NUint + 1, NDouble, NString, black_magic::S<Args1...>,
             pushed>()(cparams);
    }
};

template <typename F, int NInt, int NUint, int NDouble, int NString,
          typename... Args1, typename... Args2>
struct Call<F, NInt, NUint, NDouble, NString, black_magic::S<double, Args1...>,
            black_magic::S<Args2...>>
{
    void operator()(F cparams)
    {
        using pushed = typename black_magic::S<Args2...>::template push_back<
            CallPair<double, NDouble>>;
        Call<F, NInt, NUint, NDouble + 1, NString, black_magic::S<Args1...>,
             pushed>()(cparams);
    }
};

template <typename F, int NInt, int NUint, int NDouble, int NString,
          typename... Args1, typename... Args2>
struct Call<F, NInt, NUint, NDouble, NString,
            black_magic::S<std::string, Args1...>, black_magic::S<Args2...>>
{
    void operator()(F cparams)
    {
        using pushed = typename black_magic::S<Args2...>::template push_back<
            CallPair<std::string, NString>>;
        Call<F, NInt, NUint, NDouble, NString + 1, black_magic::S<Args1...>,
             pushed>()(cparams);
    }
};

template <typename F, int NInt, int NUint, int NDouble, int NString,
          typename... Args1>
struct Call<F, NInt, NUint, NDouble, NString, black_magic::S<>,
            black_magic::S<Args1...>>
{
    void operator()(F cparams)
    {
        cparams.handler(
            cparams.req, cparams.res,
            cparams.params.template get<typename Args1::type>(Args1::pos)...);
    }
};

template <typename Func, typename... ArgsWrapped> struct Wrapped
{
    template <typename... Args>
    void set(
        Func f,
        typename std::enable_if<
            !std::is_same<
                typename std::tuple_element<0, std::tuple<Args..., void>>::type,
                const Request&>::value,
            int>::type = 0)
    {
        handler = (
#ifdef BMCWEB_CAN_USE_CPP14
            [f = std::move(f)]
#else
            [f]
#endif
            (const Request&, Response& res, Args... args) {
                res = Response(f(args...));
                res.end();
            });
    }

    template <typename Req, typename... Args> struct ReqHandlerWrapper
    {
        ReqHandlerWrapper(Func f) : f(std::move(f))
        {
        }

        void operator()(const Request& req, Response& res, Args... args)
        {
            res = Response(f(req, args...));
            res.end();
        }

        Func f;
    };

    template <typename... Args>
    void set(
        Func f,
        typename std::enable_if<
            std::is_same<
                typename std::tuple_element<0, std::tuple<Args..., void>>::type,
                const Request&>::value &&
                !std::is_same<typename std::tuple_element<
                                  1, std::tuple<Args..., void, void>>::type,
                              Response&>::value,
            int>::type = 0)
    {
        handler = ReqHandlerWrapper<Args...>(std::move(f));
        /*handler = (
            [f = std::move(f)]
            (const Request& req, Response& res, Args... args){
                 res = Response(f(req, args...));
                 res.end();
            });*/
    }

    template <typename... Args>
    void set(
        Func f,
        typename std::enable_if<
            std::is_same<
                typename std::tuple_element<0, std::tuple<Args..., void>>::type,
                const Request&>::value &&
                std::is_same<typename std::tuple_element<
                                 1, std::tuple<Args..., void, void>>::type,
                             Response&>::value,
            int>::type = 0)
    {
        handler = std::move(f);
    }

    template <typename... Args> struct HandlerTypeHelper
    {
        using type =
            std::function<void(const crow::Request&, crow::Response&, Args...)>;
        using args_type =
            black_magic::S<typename black_magic::promote_t<Args>...>;
    };

    template <typename... Args>
    struct HandlerTypeHelper<const Request&, Args...>
    {
        using type =
            std::function<void(const crow::Request&, crow::Response&, Args...)>;
        using args_type =
            black_magic::S<typename black_magic::promote_t<Args>...>;
    };

    template <typename... Args>
    struct HandlerTypeHelper<const Request&, Response&, Args...>
    {
        using type =
            std::function<void(const crow::Request&, crow::Response&, Args...)>;
        using args_type =
            black_magic::S<typename black_magic::promote_t<Args>...>;
    };

    typename HandlerTypeHelper<ArgsWrapped...>::type handler;

    void operator()(const Request& req, Response& res,
                    const RoutingParams& params)
    {
        detail::routing_handler_call_helper::Call<
            detail::routing_handler_call_helper::CallParams<decltype(handler)>,
            0, 0, 0, 0, typename HandlerTypeHelper<ArgsWrapped...>::args_type,
            black_magic::S<>>()(
            detail::routing_handler_call_helper::CallParams<decltype(handler)>{
                handler, params, req, res});
    }
};
} // namespace routing_handler_call_helper
} // namespace detail

class WebSocketRule : public BaseRule
{
    using self_t = WebSocketRule;

  public:
    WebSocketRule(std::string rule) : BaseRule(std::move(rule))
    {
    }

    void validate() override
    {
    }

    void handle(const Request&, Response& res, const RoutingParams&) override
    {
        res = Response(boost::beast::http::status::not_found);
        res.end();
    }

    void handleUpgrade(const Request& req, Response&,
                       boost::asio::ip::tcp::socket&& adaptor) override
    {
        new crow::websocket::ConnectionImpl<boost::asio::ip::tcp::socket>(
            req, std::move(adaptor), openHandler, messageHandler, closeHandler,
            errorHandler);
    }
#ifdef BMCWEB_ENABLE_SSL
    void handleUpgrade(const Request& req, Response&,
                       boost::beast::ssl_stream<boost::asio::ip::tcp::socket>&&
                           adaptor) override
    {
        std::shared_ptr<crow::websocket::ConnectionImpl<
            boost::beast::ssl_stream<boost::asio::ip::tcp::socket>>>
            myConnection = std::make_shared<crow::websocket::ConnectionImpl<
                boost::beast::ssl_stream<boost::asio::ip::tcp::socket>>>(
                req, std::move(adaptor), openHandler, messageHandler,
                closeHandler, errorHandler);
        myConnection->start();
    }
#endif

    template <typename Func> self_t& onopen(Func f)
    {
        openHandler = f;
        return *this;
    }

    template <typename Func> self_t& onmessage(Func f)
    {
        messageHandler = f;
        return *this;
    }

    template <typename Func> self_t& onclose(Func f)
    {
        closeHandler = f;
        return *this;
    }

    template <typename Func> self_t& onerror(Func f)
    {
        errorHandler = f;
        return *this;
    }

  protected:
    std::function<void(crow::websocket::Connection&)> openHandler;
    std::function<void(crow::websocket::Connection&, const std::string&, bool)>
        messageHandler;
    std::function<void(crow::websocket::Connection&, const std::string&)>
        closeHandler;
    std::function<void(crow::websocket::Connection&)> errorHandler;
};

template <typename T> struct RuleParameterTraits
{
    using self_t = T;
    WebSocketRule& websocket()
    {
        auto p = new WebSocketRule(((self_t*)this)->rule);
        ((self_t*)this)->ruleToUpgrade.reset(p);
        return *p;
    }

    self_t& name(std::string name) noexcept
    {
        ((self_t*)this)->nameStr = std::move(name);
        return (self_t&)*this;
    }

    self_t& methods(boost::beast::http::verb method)
    {
        ((self_t*)this)->methodsBitfield = 1 << (int)method;
        return (self_t&)*this;
    }

    template <typename... MethodArgs>
    self_t& methods(boost::beast::http::verb method, MethodArgs... args_method)
    {
        methods(args_method...);
        ((self_t*)this)->methodsBitfield |= 1 << (int)method;
        return (self_t&)*this;
    }
};

class DynamicRule : public BaseRule, public RuleParameterTraits<DynamicRule>
{
  public:
    DynamicRule(std::string rule) : BaseRule(std::move(rule))
    {
    }

    void validate() override
    {
        if (!erasedHandler)
        {
            throw std::runtime_error(nameStr + (!nameStr.empty() ? ": " : "") +
                                     "no handler for url " + rule);
        }
    }

    void handle(const Request& req, Response& res,
                const RoutingParams& params) override
    {
        erasedHandler(req, res, params);
    }

    template <typename Func> void operator()(Func f)
    {
        using function_t = utility::function_traits<Func>;

        erasedHandler =
            wrap(std::move(f), black_magic::gen_seq<function_t::arity>());
    }

    // enable_if Arg1 == request && Arg2 == Response
    // enable_if Arg1 == request && Arg2 != resposne
    // enable_if Arg1 != request

    template <typename Func, unsigned... Indices>

    std::function<void(const Request&, Response&, const RoutingParams&)>
        wrap(Func f, black_magic::Seq<Indices...>)
    {
        using function_t = utility::function_traits<Func>;

        if (!black_magic::isParameterTagCompatible(
                black_magic::getParameterTagRuntime(rule.c_str()),
                black_magic::compute_parameter_tag_from_args_list<
                    typename function_t::template arg<Indices>...>::value))
        {
            throw std::runtime_error("routeDynamic: Handler type is mismatched "
                                     "with URL parameters: " +
                                     rule);
        }
        auto ret = detail::routing_handler_call_helper::Wrapped<
            Func, typename function_t::template arg<Indices>...>();
        ret.template set<typename function_t::template arg<Indices>...>(
            std::move(f));
        return ret;
    }

    template <typename Func> void operator()(std::string name, Func&& f)
    {
        nameStr = std::move(name);
        (*this).template operator()<Func>(std::forward(f));
    }

  private:
    std::function<void(const Request&, Response&, const RoutingParams&)>
        erasedHandler;
};

template <typename... Args>
class TaggedRule : public BaseRule,
                   public RuleParameterTraits<TaggedRule<Args...>>
{
  public:
    using self_t = TaggedRule<Args...>;

    TaggedRule(std::string rule) : BaseRule(std::move(rule))
    {
    }

    void validate() override
    {
        if (!handler)
        {
            throw std::runtime_error(nameStr + (!nameStr.empty() ? ": " : "") +
                                     "no handler for url " + rule);
        }
    }

    template <typename Func>
    typename std::enable_if<
        black_magic::CallHelper<Func, black_magic::S<Args...>>::value,
        void>::type
        operator()(Func&& f)
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
            "string, int, crow::resposne, nlohmann::json");

        handler = [f = std::move(f)](const Request&, Response& res,
                                     Args... args) {
            res = Response(f(args...));
            res.end();
        };
    }

    template <typename Func>
    typename std::enable_if<
        !black_magic::CallHelper<Func, black_magic::S<Args...>>::value &&
            black_magic::CallHelper<
                Func, black_magic::S<crow::Request, Args...>>::value,
        void>::type
        operator()(Func&& f)
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
            "string, int, crow::resposne,nlohmann::json");

        handler = [f = std::move(f)](const crow::Request& req,
                                     crow::Response& res, Args... args) {
            res = Response(f(req, args...));
            res.end();
        };
    }

    template <typename Func>
    typename std::enable_if<
        !black_magic::CallHelper<Func, black_magic::S<Args...>>::value &&
            !black_magic::CallHelper<
                Func, black_magic::S<crow::Request, Args...>>::value,
        void>::type
        operator()(Func&& f)
    {
        static_assert(
            black_magic::CallHelper<Func, black_magic::S<Args...>>::value ||
                black_magic::CallHelper<
                    Func, black_magic::S<crow::Request, Args...>>::value ||
                black_magic::CallHelper<
                    Func, black_magic::S<crow::Request, crow::Response&,
                                         Args...>>::value,
            "Handler type is mismatched with URL parameters");
        static_assert(
            std::is_same<void, decltype(f(std::declval<crow::Request>(),
                                          std::declval<crow::Response&>(),
                                          std::declval<Args>()...))>::value,
            "Handler function with response argument should have void return "
            "type");

        handler = std::move(f);
    }

    template <typename Func> void operator()(std::string name, Func&& f)
    {
        nameStr = std::move(name);
        (*this).template operator()<Func>(std::forward(f));
    }

    void handle(const Request& req, Response& res,
                const RoutingParams& params) override
    {
        detail::routing_handler_call_helper::Call<
            detail::routing_handler_call_helper::CallParams<decltype(handler)>,
            0, 0, 0, 0, black_magic::S<Args...>, black_magic::S<>>()(
            detail::routing_handler_call_helper::CallParams<decltype(handler)>{
                handler, params, req, res});
    }

  private:
    std::function<void(const crow::Request&, crow::Response&, Args...)> handler;
};

const int ruleSpecialRedirectSlash = 1;

class Trie
{
  public:
    struct Node
    {
        unsigned ruleIndex{};
        std::array<unsigned, (int)ParamType::MAX> paramChildrens{};
        boost::container::flat_map<std::string, unsigned> children;

        bool isSimpleNode() const
        {
            return !ruleIndex && std::all_of(std::begin(paramChildrens),
                                             std::end(paramChildrens),
                                             [](unsigned x) { return !x; });
        }
    };

    Trie() : nodes(1)
    {
    }

  private:
    void optimizeNode(Node* node)
    {
        for (auto x : node->paramChildrens)
        {
            if (!x)
                continue;
            Node* child = &nodes[x];
            optimizeNode(child);
        }
        if (node->children.empty())
            return;
        bool mergeWithChild = true;
        for (auto& kv : node->children)
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
            for (auto& kv : node->children)
            {
                Node* child = &nodes[kv.second];
                for (auto& childKv : child->children)
                {
                    merged[kv.first + childKv.first] = childKv.second;
                }
            }
            node->children = std::move(merged);
            optimizeNode(node);
        }
        else
        {
            for (auto& kv : node->children)
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
        if (!head()->isSimpleNode())
            throw std::runtime_error(
                "Internal error: Trie header should be simple!");
        optimize();
    }

    void findRouteIndexes(const std::string& req_url,
                          std::vector<unsigned>& route_indexes,
                          const Node* node = nullptr, unsigned pos = 0)
    {
        if (node == nullptr)
        {
            node = head();
        }
        for (auto& kv : node->children)
        {
            const std::string& fragment = kv.first;
            const Node* child = &nodes[kv.second];
            if (pos >= req_url.size())
            {
                if (child->ruleIndex != 0 && fragment != "/")
                {
                    route_indexes.push_back(child->ruleIndex);
                }
                findRouteIndexes(req_url, route_indexes, child,
                                 pos + fragment.size());
            }
            else
            {
                if (req_url.compare(pos, fragment.size(), fragment) == 0)
                {
                    findRouteIndexes(req_url, route_indexes, child,
                                     pos + fragment.size());
                }
            }
        }
    }

    std::pair<unsigned, RoutingParams>
        find(const boost::string_view req_url, const Node* node = nullptr,
             unsigned pos = 0, RoutingParams* params = nullptr) const
    {
        RoutingParams empty;
        if (params == nullptr)
            params = &empty;

        unsigned found{};
        RoutingParams matchParams;

        if (node == nullptr)
            node = head();
        if (pos == req_url.size())
            return {node->ruleIndex, *params};

        auto updateFound =
            [&found, &matchParams](std::pair<unsigned, RoutingParams>& ret) {
                if (ret.first && (!found || found > ret.first))
                {
                    found = ret.first;
                    matchParams = std::move(ret.second);
                }
            };

        if (node->paramChildrens[(int)ParamType::INT])
        {
            char c = req_url[pos];
            if ((c >= '0' && c <= '9') || c == '+' || c == '-')
            {
                char* eptr;
                errno = 0;
                long long int value =
                    std::strtoll(req_url.data() + pos, &eptr, 10);
                if (errno != ERANGE && eptr != req_url.data() + pos)
                {
                    params->intParams.push_back(value);
                    auto ret =
                        find(req_url,
                             &nodes[node->paramChildrens[(int)ParamType::INT]],
                             eptr - req_url.data(), params);
                    updateFound(ret);
                    params->intParams.pop_back();
                }
            }
        }

        if (node->paramChildrens[(int)ParamType::UINT])
        {
            char c = req_url[pos];
            if ((c >= '0' && c <= '9') || c == '+')
            {
                char* eptr;
                errno = 0;
                unsigned long long int value =
                    std::strtoull(req_url.data() + pos, &eptr, 10);
                if (errno != ERANGE && eptr != req_url.data() + pos)
                {
                    params->uintParams.push_back(value);
                    auto ret =
                        find(req_url,
                             &nodes[node->paramChildrens[(int)ParamType::UINT]],
                             eptr - req_url.data(), params);
                    updateFound(ret);
                    params->uintParams.pop_back();
                }
            }
        }

        if (node->paramChildrens[(int)ParamType::DOUBLE])
        {
            char c = req_url[pos];
            if ((c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.')
            {
                char* eptr;
                errno = 0;
                double value = std::strtod(req_url.data() + pos, &eptr);
                if (errno != ERANGE && eptr != req_url.data() + pos)
                {
                    params->doubleParams.push_back(value);
                    auto ret = find(
                        req_url,
                        &nodes[node->paramChildrens[(int)ParamType::DOUBLE]],
                        eptr - req_url.data(), params);
                    updateFound(ret);
                    params->doubleParams.pop_back();
                }
            }
        }

        if (node->paramChildrens[(int)ParamType::STRING])
        {
            size_t epos = pos;
            for (; epos < req_url.size(); epos++)
            {
                if (req_url[epos] == '/')
                    break;
            }

            if (epos != pos)
            {
                params->stringParams.emplace_back(
                    req_url.substr(pos, epos - pos));
                auto ret =
                    find(req_url,
                         &nodes[node->paramChildrens[(int)ParamType::STRING]],
                         epos, params);
                updateFound(ret);
                params->stringParams.pop_back();
            }
        }

        if (node->paramChildrens[(int)ParamType::PATH])
        {
            size_t epos = req_url.size();

            if (epos != pos)
            {
                params->stringParams.emplace_back(
                    req_url.substr(pos, epos - pos));
                auto ret = find(
                    req_url, &nodes[node->paramChildrens[(int)ParamType::PATH]],
                    epos, params);
                updateFound(ret);
                params->stringParams.pop_back();
            }
        }

        for (auto& kv : node->children)
        {
            const std::string& fragment = kv.first;
            const Node* child = &nodes[kv.second];

            if (req_url.compare(pos, fragment.size(), fragment) == 0)
            {
                auto ret = find(req_url, child, pos + fragment.size(), params);
                updateFound(ret);
            }
        }

        return {found, matchParams};
    }

    void add(const std::string& url, unsigned ruleIndex)
    {
        unsigned idx{0};

        for (unsigned i = 0; i < url.size(); i++)
        {
            char c = url[i];
            if (c == '<')
            {
                static struct ParamTraits
                {
                    ParamType type;
                    std::string name;
                } paramTraits[] = {
                    {ParamType::INT, "<int>"},
                    {ParamType::UINT, "<uint>"},
                    {ParamType::DOUBLE, "<float>"},
                    {ParamType::DOUBLE, "<double>"},
                    {ParamType::STRING, "<str>"},
                    {ParamType::STRING, "<string>"},
                    {ParamType::PATH, "<path>"},
                };

                for (auto& x : paramTraits)
                {
                    if (url.compare(i, x.name.size(), x.name) == 0)
                    {
                        if (!nodes[idx].paramChildrens[(int)x.type])
                        {
                            auto newNodeIdx = newNode();
                            nodes[idx].paramChildrens[(int)x.type] = newNodeIdx;
                        }
                        idx = nodes[idx].paramChildrens[(int)x.type];
                        i += x.name.size();
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
                    auto newNodeIdx = newNode();
                    nodes[idx].children.emplace(piece, newNodeIdx);
                }
                idx = nodes[idx].children[piece];
            }
        }
        if (nodes[idx].ruleIndex)
            throw std::runtime_error("handler already exists for " + url);
        nodes[idx].ruleIndex = ruleIndex;
    }

  private:
    void debugNodePrint(Node* n, int level)
    {
        for (int i = 0; i < (int)ParamType::MAX; i++)
        {
            if (n->paramChildrens[i])
            {
                BMCWEB_LOG_DEBUG << std::string(
                    2 * level, ' ') /*<< "("<<n->paramChildrens[i]<<") "*/;
                switch ((ParamType)i)
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
                    default:
                        BMCWEB_LOG_DEBUG << "<ERROR>";
                        break;
                }

                debugNodePrint(&nodes[n->paramChildrens[i]], level + 1);
            }
        }
        for (auto& kv : n->children)
        {
            BMCWEB_LOG_DEBUG
                << std::string(2 * level, ' ') /*<< "(" << kv.second << ") "*/
                << kv.first;
            debugNodePrint(&nodes[kv.second], level + 1);
        }
    }

  public:
    void debugPrint()
    {
        debugNodePrint(head(), 0);
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
        return nodes.size() - 1;
    }

    std::vector<Node> nodes;
};

class Router
{
  public:
    Router() : rules(2)
    {
    }

    DynamicRule& newRuleDynamic(const std::string& rule)
    {
        std::unique_ptr<DynamicRule> ruleObject =
            std::make_unique<DynamicRule>(rule);
        DynamicRule* ptr = ruleObject.get();
        internalAddRuleObject(rule, std::move(ruleObject));

        return *ptr;
    }

    template <uint64_t N>
    typename black_magic::Arguments<N>::type::template rebind<TaggedRule>&
        newRuleTagged(const std::string& rule)
    {
        using RuleT = typename black_magic::Arguments<N>::type::template rebind<
            TaggedRule>;
        std::unique_ptr<RuleT> ruleObject = std::make_unique<RuleT>(rule);
        RuleT* ptr = ruleObject.get();

        internalAddRuleObject(rule, std::move(ruleObject));

        return *ptr;
    }

    void internalAddRuleObject(const std::string& rule,
                               std::unique_ptr<BaseRule> ruleObject)
    {
        rules.emplace_back(std::move(ruleObject));
        trie.add(rule, rules.size() - 1);

        // directory case:
        //   request to `/about' url matches `/about/' rule
        if (rule.size() > 2 && rule.back() == '/')
        {
            trie.add(rule.substr(0, rule.size() - 1), rules.size() - 1);
        }
    }

    void validate()
    {
        trie.validate();
        for (auto& rule : rules)
        {
            if (rule)
            {
                auto upgraded = rule->upgrade();
                if (upgraded)
                    rule = std::move(upgraded);
                rule->validate();
            }
        }
    }

    template <typename Adaptor>
    void handleUpgrade(const Request& req, Response& res, Adaptor&& adaptor)
    {
        auto found = trie.find(req.url);
        unsigned ruleIndex = found.first;
        if (!ruleIndex)
        {
            BMCWEB_LOG_DEBUG << "Cannot match rules " << req.url;
            res = Response(boost::beast::http::status::not_found);
            res.end();
            return;
        }

        if (ruleIndex >= rules.size())
            throw std::runtime_error("Trie internal structure corrupted!");

        if (ruleIndex == ruleSpecialRedirectSlash)
        {
            BMCWEB_LOG_INFO << "Redirecting to a url with trailing slash: "
                            << req.url;
            res = Response(boost::beast::http::status::moved_permanently);

            // TODO absolute url building
            if (req.getHeaderValue("Host").empty())
            {
                res.addHeader("Location", std::string(req.url) + "/");
            }
            else
            {
                res.addHeader(
                    "Location",
                    req.isSecure
                        ? "https://"
                        : "http://" + std::string(req.getHeaderValue("Host")) +
                              std::string(req.url) + "/");
            }
            res.end();
            return;
        }

        if ((rules[ruleIndex]->getMethods() & (1 << (uint32_t)req.method())) ==
            0)
        {
            BMCWEB_LOG_DEBUG << "Rule found but method mismatch: " << req.url
                             << " with " << req.methodString() << "("
                             << (uint32_t)req.method() << ") / "
                             << rules[ruleIndex]->getMethods();
            res = Response(boost::beast::http::status::not_found);
            res.end();
            return;
        }

        BMCWEB_LOG_DEBUG << "Matched rule (upgrade) '" << rules[ruleIndex]->rule
                         << "' " << (uint32_t)req.method() << " / "
                         << rules[ruleIndex]->getMethods();

        // any uncaught exceptions become 500s
        try
        {
            rules[ruleIndex]->handleUpgrade(req, res, std::move(adaptor));
        }
        catch (std::exception& e)
        {
            BMCWEB_LOG_ERROR << "An uncaught exception occurred: " << e.what();
            res = Response(boost::beast::http::status::internal_server_error);
            res.end();
            return;
        }
        catch (...)
        {
            BMCWEB_LOG_ERROR
                << "An uncaught exception occurred. The type was unknown "
                   "so no information was available.";
            res = Response(boost::beast::http::status::internal_server_error);
            res.end();
            return;
        }
    }

    void handle(const Request& req, Response& res)
    {
        auto found = trie.find(req.url);

        unsigned ruleIndex = found.first;

        if (!ruleIndex)
        {
            BMCWEB_LOG_DEBUG << "Cannot match rules " << req.url;
            res.result(boost::beast::http::status::not_found);
            res.end();
            return;
        }

        if (ruleIndex >= rules.size())
            throw std::runtime_error("Trie internal structure corrupted!");

        if (ruleIndex == ruleSpecialRedirectSlash)
        {
            BMCWEB_LOG_INFO << "Redirecting to a url with trailing slash: "
                            << req.url;
            res = Response(boost::beast::http::status::moved_permanently);

            // TODO absolute url building
            if (req.getHeaderValue("Host").empty())
            {
                res.addHeader("Location", std::string(req.url) + "/");
            }
            else
            {
                res.addHeader("Location",
                              (req.isSecure ? "https://" : "http://") +
                                  std::string(req.getHeaderValue("Host")) +
                                  std::string(req.url) + "/");
            }
            res.end();
            return;
        }

        if ((rules[ruleIndex]->getMethods() & (1 << (uint32_t)req.method())) ==
            0)
        {
            BMCWEB_LOG_DEBUG << "Rule found but method mismatch: " << req.url
                             << " with " << req.methodString() << "("
                             << (uint32_t)req.method() << ") / "
                             << rules[ruleIndex]->getMethods();
            res = Response(boost::beast::http::status::not_found);
            res.end();
            return;
        }

        BMCWEB_LOG_DEBUG << "Matched rule '" << rules[ruleIndex]->rule << "' "
                         << (uint32_t)req.method() << " / "
                         << rules[ruleIndex]->getMethods();

        // any uncaught exceptions become 500s
        try
        {
            rules[ruleIndex]->handle(req, res, found.second);
        }
        catch (std::exception& e)
        {
            BMCWEB_LOG_ERROR << "An uncaught exception occurred: " << e.what();
            res = Response(boost::beast::http::status::internal_server_error);
            res.end();
            return;
        }
        catch (...)
        {
            BMCWEB_LOG_ERROR
                << "An uncaught exception occurred. The type was unknown "
                   "so no information was available.";
            res = Response(boost::beast::http::status::internal_server_error);
            res.end();
            return;
        }
    }

    void debugPrint()
    {
        trie.debugPrint();
    }

    std::vector<const std::string*> getRoutes(const std::string& parent)
    {
        std::vector<unsigned> x;
        std::vector<const std::string*> ret;
        trie.findRouteIndexes(parent, x);
        for (unsigned index : x)
        {
            ret.push_back(&rules[index]->rule);
        }
        return ret;
    }

  private:
    std::vector<std::unique_ptr<BaseRule>> rules;
    Trie trie;
};
} // namespace crow
