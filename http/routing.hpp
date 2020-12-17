#pragma once

#include "common.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "privileges.hpp"
#include "sessions.hpp"
#include "utility.hpp"
#include "websocket.hpp"

#include <async_resp.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/lexical_cast.hpp>

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

namespace crow
{

class BaseRule
{
  public:
    BaseRule(const std::string& thisRule) : rule(thisRule)
    {}

    virtual ~BaseRule() = default;

    virtual void validate() = 0;
    std::unique_ptr<BaseRule> upgrade()
    {
        if (ruleToUpgrade)
        {
            return std::move(ruleToUpgrade);
        }
        return {};
    }

    virtual void handle(const Request&, Response&, const RoutingParams&) = 0;
    virtual void handleUpgrade(const Request&, Response& res,
                               boost::asio::ip::tcp::socket&&)
    {
        res.result(boost::beast::http::status::not_found);
        res.end();
    }
#ifdef BMCWEB_ENABLE_SSL
    virtual void
        handleUpgrade(const Request&, Response& res,
                      boost::beast::ssl_stream<boost::asio::ip::tcp::socket>&&)
    {
        res.result(boost::beast::http::status::not_found);
        res.end();
    }
#endif

    size_t getMethods()
    {
        return methodsBitfield;
    }

    bool checkPrivileges(const redfish::Privileges& userPrivileges)
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

    size_t methodsBitfield{
        1 << static_cast<size_t>(boost::beast::http::verb::get)};

    std::vector<redfish::Privileges> privilegesSet;

    std::string rule;
    std::string nameStr;

    std::unique_ptr<BaseRule> ruleToUpgrade;

    friend class Router;
    template <typename T>
    friend struct RuleParameterTraits;
};

namespace detail
{
namespace routing_handler_call_helper
{
template <typename T, int Pos>
struct CallPair
{
    using type = T;
    static const int pos = Pos;
};

template <typename H1>
struct CallParams
{
    H1& handler;
    const RoutingParams& params;
    const Request& req;
    Response& res;
};

template <typename F, int NInt, int NUint, int NDouble, int NString,
          typename S1, typename S2>
struct Call
{};

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

template <typename Func, typename... ArgsWrapped>
struct Wrapped
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
        handler = [f = std::move(f)](const Request&, Response& res,
                                     Args... args) {
            res.result(f(args...));
            res.end();
        };
    }

    template <typename Req, typename... Args>
    struct ReqHandlerWrapper
    {
        ReqHandlerWrapper(Func fIn) : f(std::move(fIn))
        {}

        void operator()(const Request& req, Response& res, Args... args)
        {
            res.result(f(req, args...));
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
                 res.result(f(req, args...));
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

    template <typename... Args>
    struct HandlerTypeHelper
    {
        using type =
            std::function<void(const crow::Request&, crow::Response&, Args...)>;
        using args_type =
            black_magic::S<typename black_magic::PromoteT<Args>...>;
    };

    template <typename... Args>
    struct HandlerTypeHelper<const Request&, Args...>
    {
        using type =
            std::function<void(const crow::Request&, crow::Response&, Args...)>;
        using args_type =
            black_magic::S<typename black_magic::PromoteT<Args>...>;
    };

    template <typename... Args>
    struct HandlerTypeHelper<const Request&, Response&, Args...>
    {
        using type =
            std::function<void(const crow::Request&, crow::Response&, Args...)>;
        using args_type =
            black_magic::S<typename black_magic::PromoteT<Args>...>;
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
    WebSocketRule(const std::string& ruleIn) : BaseRule(ruleIn)
    {}

    void validate() override
    {}

    void handle(const Request&, Response& res, const RoutingParams&) override
    {
        res.result(boost::beast::http::status::not_found);
        res.end();
    }

    void handleUpgrade(const Request& req, Response&,
                       boost::asio::ip::tcp::socket&& adaptor) override
    {
        std::shared_ptr<
            crow::websocket::ConnectionImpl<boost::asio::ip::tcp::socket>>
            myConnection = std::make_shared<
                crow::websocket::ConnectionImpl<boost::asio::ip::tcp::socket>>(
                req, std::move(adaptor), openHandler, messageHandler,
                closeHandler, errorHandler);
        myConnection->start();
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

    template <typename Func>
    self_t& onopen(Func f)
    {
        openHandler = f;
        return *this;
    }

    template <typename Func>
    self_t& onmessage(Func f)
    {
        messageHandler = f;
        return *this;
    }

    template <typename Func>
    self_t& onclose(Func f)
    {
        closeHandler = f;
        return *this;
    }

    template <typename Func>
    self_t& onerror(Func f)
    {
        errorHandler = f;
        return *this;
    }

  protected:
    std::function<void(crow::websocket::Connection&,
                       std::shared_ptr<bmcweb::AsyncResp>)>
        openHandler;
    std::function<void(crow::websocket::Connection&, const std::string&, bool)>
        messageHandler;
    std::function<void(crow::websocket::Connection&, const std::string&)>
        closeHandler;
    std::function<void(crow::websocket::Connection&)> errorHandler;
};

template <typename T>
struct RuleParameterTraits
{
    using self_t = T;
    WebSocketRule& websocket()
    {
        self_t* self = static_cast<self_t*>(this);
        WebSocketRule* p = new WebSocketRule(self->rule);
        self->ruleToUpgrade.reset(p);
        return *p;
    }

    self_t& name(const std::string_view name) noexcept
    {
        self_t* self = static_cast<self_t*>(this);
        self->nameStr = name;
        return *self;
    }

    self_t& methods(boost::beast::http::verb method)
    {
        self_t* self = static_cast<self_t*>(this);
        self->methodsBitfield = 1U << static_cast<size_t>(method);
        return *self;
    }

    template <typename... MethodArgs>
    self_t& methods(boost::beast::http::verb method, MethodArgs... argsMethod)
    {
        self_t* self = static_cast<self_t*>(this);
        methods(argsMethod...);
        self->methodsBitfield |= 1U << static_cast<size_t>(method);
        return *self;
    }

    template <typename... MethodArgs>
    self_t& privileges(std::initializer_list<const char*> l)
    {
        self_t* self = static_cast<self_t*>(this);
        self->privilegesSet.emplace_back(l);
        return *self;
    }

    template <typename... MethodArgs>
    self_t& privileges(const std::vector<redfish::Privileges>& p)
    {
        self_t* self = static_cast<self_t*>(this);
        for (const redfish::Privileges& privilege : p)
        {
            self->privilegesSet.emplace_back(privilege);
        }
        return *self;
    }
};

class DynamicRule : public BaseRule, public RuleParameterTraits<DynamicRule>
{
  public:
    DynamicRule(const std::string& ruleIn) : BaseRule(ruleIn)
    {}

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

    template <typename Func>
    void operator()(Func f)
    {
        using function_t = utility::function_traits<Func>;
        erasedHandler =
            wrap(std::move(f),
                 std::make_integer_sequence<unsigned, function_t::arity>{});
    }

    // enable_if Arg1 == request && Arg2 == Response
    // enable_if Arg1 == request && Arg2 != response
    // enable_if Arg1 != request

    template <typename Func, unsigned... Indices>
    std::function<void(const Request&, Response&, const RoutingParams&)>
        wrap(Func f, std::integer_sequence<unsigned, Indices...>)
    {
        using function_t = crow::utility::function_traits<Func>;

        if (!black_magic::isParameterTagCompatible(
                black_magic::getParameterTag(rule.c_str()),
                black_magic::computeParameterTagFromArgsList<
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

    template <typename Func>
    void operator()(std::string name, Func&& f)
    {
        nameStr = std::move(name);
        (*this).template operator()<Func>(std::forward(f));
    }

  private:
    std::function<void(const Request&, Response&, const RoutingParams&)>
        erasedHandler;
};

template <typename... Args>
class TaggedRule :
    public BaseRule,
    public RuleParameterTraits<TaggedRule<Args...>>
{
  public:
    using self_t = TaggedRule<Args...>;

    TaggedRule(const std::string& ruleIn) : BaseRule(ruleIn)
    {}

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
            "string, int, crow::response, nlohmann::json");

        handler = [f = std::move(f)](const Request&, Response& res,
                                     Args... args) {
            res.result(f(args...));
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
            "string, int, crow::response,nlohmann::json");

        handler = [f = std::move(f)](const crow::Request& req,
                                     crow::Response& res, Args... args) {
            res.result(f(req, args...));
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
            "Handler function with response argument should have void "
            "return "
            "type");

        handler = std::move(f);
    }

    template <typename Func>
    void operator()(const std::string_view name, Func&& f)
    {
        nameStr = name;
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
        std::array<size_t, static_cast<size_t>(ParamType::MAX)>
            paramChildrens{};
        boost::container::flat_map<std::string, unsigned> children;

        bool isSimpleNode() const
        {
            return !ruleIndex && std::all_of(std::begin(paramChildrens),
                                             std::end(paramChildrens),
                                             [](size_t x) { return !x; });
        }
    };

    Trie() : nodes(1)
    {}

  private:
    void optimizeNode(Node* node)
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

    void optimize()
    {
        optimizeNode(head());
    }

  public:
    void validate()
    {
        if (!head()->isSimpleNode())
        {
            throw std::runtime_error(
                "Internal error: Trie header should be simple!");
        }
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
        find(const std::string_view reqUrl, const Node* node = nullptr,
             size_t pos = 0, RoutingParams* params = nullptr) const
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

    void add(const std::string& url, unsigned ruleIndex)
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

  private:
    void debugNodePrint(Node* n, size_t level)
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
    typename black_magic::Arguments<N>::type::template rebind<TaggedRule>&
        newRuleTagged(const std::string& rule)
    {
        using RuleT = typename black_magic::Arguments<N>::type::template rebind<
            TaggedRule>;
        std::unique_ptr<RuleT> ruleObject = std::make_unique<RuleT>(rule);
        RuleT* ptr = ruleObject.get();
        allRules.emplace_back(std::move(ruleObject));

        return *ptr;
    }

    void internalAddRuleObject(const std::string& rule, BaseRule* ruleObject)
    {
        if (ruleObject == nullptr)
        {
            return;
        }
        for (size_t method = 0, methodBit = 1; method < maxHttpVerbCount;
             method++, methodBit <<= 1)
        {
            if (ruleObject->methodsBitfield & methodBit)
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

    template <typename Adaptor>
    void handleUpgrade(const Request& req, Response& res, Adaptor&& adaptor)
    {
        if (static_cast<size_t>(req.method()) >= perMethods.size())
        {
            res.result(boost::beast::http::status::not_found);
            res.end();
            return;
        }

        PerMethod& perMethod = perMethods[static_cast<size_t>(req.method())];
        Trie& trie = perMethod.trie;
        std::vector<BaseRule*>& rules = perMethod.rules;

        const std::pair<unsigned, RoutingParams>& found = trie.find(req.url);
        unsigned ruleIndex = found.first;
        if (!ruleIndex)
        {
            BMCWEB_LOG_DEBUG << "Cannot match rules " << req.url;
            res.result(boost::beast::http::status::not_found);
            res.end();
            return;
        }

        if (ruleIndex >= rules.size())
        {
            throw std::runtime_error("Trie internal structure corrupted!");
        }

        if (ruleIndex == ruleSpecialRedirectSlash)
        {
            BMCWEB_LOG_INFO << "Redirecting to a url with trailing slash: "
                            << req.url;
            res.result(boost::beast::http::status::moved_permanently);

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

        if ((rules[ruleIndex]->getMethods() &
             (1U << static_cast<size_t>(req.method()))) == 0)
        {
            BMCWEB_LOG_DEBUG << "Rule found but method mismatch: " << req.url
                             << " with " << req.methodString() << "("
                             << static_cast<uint32_t>(req.method()) << ") / "
                             << rules[ruleIndex]->getMethods();
            res.result(boost::beast::http::status::not_found);
            res.end();
            return;
        }

        BMCWEB_LOG_DEBUG << "Matched rule (upgrade) '" << rules[ruleIndex]->rule
                         << "' " << static_cast<uint32_t>(req.method()) << " / "
                         << rules[ruleIndex]->getMethods();

        // any uncaught exceptions become 500s
        try
        {
            rules[ruleIndex]->handleUpgrade(req, res, std::move(adaptor));
        }
        catch (std::exception& e)
        {
            BMCWEB_LOG_ERROR << "An uncaught exception occurred: " << e.what();
            res.result(boost::beast::http::status::internal_server_error);
            res.end();
            return;
        }
        catch (...)
        {
            BMCWEB_LOG_ERROR
                << "An uncaught exception occurred. The type was unknown "
                   "so no information was available.";
            res.result(boost::beast::http::status::internal_server_error);
            res.end();
            return;
        }
    }

    void handle(Request& req, Response& res)
    {
        if (static_cast<size_t>(req.method()) >= perMethods.size())
        {
            res.result(boost::beast::http::status::not_found);
            res.end();
            return;
        }
        PerMethod& perMethod = perMethods[static_cast<size_t>(req.method())];
        Trie& trie = perMethod.trie;
        std::vector<BaseRule*>& rules = perMethod.rules;

        const std::pair<unsigned, RoutingParams>& found = trie.find(req.url);

        unsigned ruleIndex = found.first;

        if (!ruleIndex)
        {
            // Check to see if this url exists at any verb
            for (const PerMethod& p : perMethods)
            {
                const std::pair<unsigned, RoutingParams>& found2 =
                    p.trie.find(req.url);
                if (found2.first > 0)
                {
                    res.result(boost::beast::http::status::method_not_allowed);
                    res.end();
                    return;
                }
            }
            BMCWEB_LOG_DEBUG << "Cannot match rules " << req.url;
            res.result(boost::beast::http::status::not_found);
            res.end();
            return;
        }

        if (ruleIndex >= rules.size())
        {
            throw std::runtime_error("Trie internal structure corrupted!");
        }

        if (ruleIndex == ruleSpecialRedirectSlash)
        {
            BMCWEB_LOG_INFO << "Redirecting to a url with trailing slash: "
                            << req.url;
            res.result(boost::beast::http::status::moved_permanently);

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

        if ((rules[ruleIndex]->getMethods() &
             (1U << static_cast<uint32_t>(req.method()))) == 0)
        {
            BMCWEB_LOG_DEBUG << "Rule found but method mismatch: " << req.url
                             << " with " << req.methodString() << "("
                             << static_cast<uint32_t>(req.method()) << ") / "
                             << rules[ruleIndex]->getMethods();
            res.result(boost::beast::http::status::method_not_allowed);
            res.end();
            return;
        }

        BMCWEB_LOG_DEBUG << "Matched rule '" << rules[ruleIndex]->rule << "' "
                         << static_cast<uint32_t>(req.method()) << " / "
                         << rules[ruleIndex]->getMethods();

        if (req.session == nullptr)
        {
            rules[ruleIndex]->handle(req, res, found.second);
            return;
        }

        crow::connections::systemBus->async_method_call(
            [&req, &res, &rules, ruleIndex, found](
                const boost::system::error_code ec,
                std::map<std::string, std::variant<bool, std::string,
                                                   std::vector<std::string>>>
                    userInfo) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "GetUserInfo failed...";
                    res.result(
                        boost::beast::http::status::internal_server_error);
                    res.end();
                    return;
                }

                const std::string* userRolePtr = nullptr;
                auto userInfoIter = userInfo.find("UserPrivilege");
                if (userInfoIter != userInfo.end())
                {
                    userRolePtr =
                        std::get_if<std::string>(&userInfoIter->second);
                }

                std::string userRole{};
                if (userRolePtr != nullptr)
                {
                    userRole = *userRolePtr;
                    BMCWEB_LOG_DEBUG << "userName = " << req.session->username
                                     << " userRole = " << *userRolePtr;
                }

                bool* remoteUserPtr = nullptr;
                auto remoteUserIter = userInfo.find("RemoteUser");
                if (remoteUserIter != userInfo.end())
                {
                    remoteUserPtr = std::get_if<bool>(&remoteUserIter->second);
                }
                if (remoteUserPtr == nullptr)
                {
                    BMCWEB_LOG_ERROR
                        << "RemoteUser property missing or wrong type";
                    res.result(
                        boost::beast::http::status::internal_server_error);
                    res.end();
                    return;
                }
                bool remoteUser = *remoteUserPtr;

                bool passwordExpired = false; // default for remote user
                if (!remoteUser)
                {
                    bool* passwordExpiredPtr = nullptr;
                    auto passwordExpiredIter =
                        userInfo.find("UserPasswordExpired");
                    if (passwordExpiredIter != userInfo.end())
                    {
                        passwordExpiredPtr =
                            std::get_if<bool>(&passwordExpiredIter->second);
                    }
                    if (passwordExpiredPtr != nullptr)
                    {
                        passwordExpired = *passwordExpiredPtr;
                    }
                    else
                    {
                        BMCWEB_LOG_ERROR
                            << "UserPasswordExpired property is expected for"
                               " local user but is missing or wrong type";
                        res.result(
                            boost::beast::http::status::internal_server_error);
                        res.end();
                        return;
                    }
                }

                // Get the userprivileges from the role
                redfish::Privileges userPrivileges =
                    redfish::getUserPrivileges(userRole);

                // Set isConfigureSelfOnly based on D-Bus results.  This
                // ignores the results from both pamAuthenticateUser and the
                // value from any previous use of this session.
                req.session->isConfigureSelfOnly = passwordExpired;

                // Modifyprivileges if isConfigureSelfOnly.
                if (req.session->isConfigureSelfOnly)
                {
                    // Remove allprivileges except ConfigureSelf
                    userPrivileges = userPrivileges.intersection(
                        redfish::Privileges{"ConfigureSelf"});
                    BMCWEB_LOG_DEBUG << "Operation limited to ConfigureSelf";
                }

                if (!rules[ruleIndex]->checkPrivileges(userPrivileges))
                {
                    res.result(boost::beast::http::status::forbidden);
                    if (req.session->isConfigureSelfOnly)
                    {
                        redfish::messages::passwordChangeRequired(
                            res, "/redfish/v1/AccountService/Accounts/" +
                                     req.session->username);
                    }
                    res.end();
                    return;
                }

                req.userRole = userRole;

                rules[ruleIndex]->handle(req, res, found.second);
            },
            "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
            "xyz.openbmc_project.User.Manager", "GetUserInfo",
            req.session->username);
    }

    void debugPrint()
    {
        for (size_t i = 0; i < perMethods.size(); i++)
        {
            BMCWEB_LOG_DEBUG << boost::beast::http::to_string(
                static_cast<boost::beast::http::verb>(i));
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
        // rule index 0, 1 has special meaning; preallocate it to avoid
        // duplication.
        PerMethod() : rules(2)
        {}
    };

    const static size_t maxHttpVerbCount =
        static_cast<size_t>(boost::beast::http::verb::unlink);

    std::array<PerMethod, maxHttpVerbCount> perMethods;
    std::vector<std::unique_ptr<BaseRule>> allRules;
};
} // namespace crow
