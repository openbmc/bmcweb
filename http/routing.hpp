#pragma once

#include "async_resp.hpp"
#include "common.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "privileges.hpp"
#include "sessions.hpp"
#include "utility.hpp"
#include "utils/dbus_utils.hpp"
#include "verb.hpp"
#include "websocket.hpp"

#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/container/flat_map.hpp>
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

class BaseRule
{
  public:
    explicit BaseRule(const std::string& thisRule) : rule(thisRule)
    {}

    virtual ~BaseRule() = default;

    BaseRule(const BaseRule&) = delete;
    BaseRule(BaseRule&&) = delete;
    BaseRule& operator=(const BaseRule&) = delete;
    BaseRule& operator=(const BaseRule&&) = delete;

    virtual void validate() = 0;
    std::unique_ptr<BaseRule> upgrade()
    {
        if (ruleToUpgrade)
        {
            return std::move(ruleToUpgrade);
        }
        return {};
    }

    virtual void handle(const Request& /*req*/,
                        const std::shared_ptr<bmcweb::AsyncResp>&,
                        const RoutingParams&) = 0;
    virtual void
        handleUpgrade(const Request& /*req*/,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      boost::asio::ip::tcp::socket&& /*adaptor*/)
    {
        asyncResp->res.result(boost::beast::http::status::not_found);
    }
#ifdef BMCWEB_ENABLE_SSL
    virtual void handleUpgrade(
        const Request& /*req*/,
        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
        boost::beast::ssl_stream<boost::asio::ip::tcp::socket>&& /*adaptor*/)
    {
        asyncResp->res.result(boost::beast::http::status::not_found);
    }
#endif

    size_t getMethods() const
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

    size_t methodsBitfield{1 << static_cast<size_t>(HttpVerb::Get)};
    static_assert(std::numeric_limits<decltype(methodsBitfield)>::digits >
                      methodNotAllowedIndex,
                  "Not enough bits to store bitfield");

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
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp;
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
            cparams.req, cparams.asyncResp,
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
            int>::type /*enable*/
        = 0)
    {
        handler = [f = std::forward<Func>(f)](
                      const Request&,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      Args... args) { asyncResp->res.result(f(args...)); };
    }

    template <typename Req, typename... Args>
    struct ReqHandlerWrapper
    {
        explicit ReqHandlerWrapper(Func fIn) : f(std::move(fIn))
        {}

        void operator()(const Request& req,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        Args... args)
        {
            asyncResp->res.result(f(req, args...));
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
                              const std::shared_ptr<bmcweb::AsyncResp>&>::value,
            int>::type /*enable*/
        = 0)
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
                             const std::shared_ptr<bmcweb::AsyncResp>&>::value,
            int>::type /*enable*/
        = 0)
    {
        handler = std::move(f);
    }

    template <typename... Args>
    struct HandlerTypeHelper
    {
        using type = std::function<void(
            const crow::Request& /*req*/,
            const std::shared_ptr<bmcweb::AsyncResp>&, Args...)>;
        using args_type =
            black_magic::S<typename black_magic::PromoteT<Args>...>;
    };

    template <typename... Args>
    struct HandlerTypeHelper<const Request&, Args...>
    {
        using type = std::function<void(
            const crow::Request& /*req*/,
            const std::shared_ptr<bmcweb::AsyncResp>&, Args...)>;
        using args_type =
            black_magic::S<typename black_magic::PromoteT<Args>...>;
    };

    template <typename... Args>
    struct HandlerTypeHelper<const Request&,
                             const std::shared_ptr<bmcweb::AsyncResp>&, Args...>
    {
        using type = std::function<void(
            const crow::Request& /*req*/,
            const std::shared_ptr<bmcweb::AsyncResp>&, Args...)>;
        using args_type =
            black_magic::S<typename black_magic::PromoteT<Args>...>;
    };

    typename HandlerTypeHelper<ArgsWrapped...>::type handler;

    void operator()(const Request& req,
                    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                    const RoutingParams& params)
    {
        detail::routing_handler_call_helper::Call<
            detail::routing_handler_call_helper::CallParams<decltype(handler)>,
            0, 0, 0, 0, typename HandlerTypeHelper<ArgsWrapped...>::args_type,
            black_magic::S<>>()(
            detail::routing_handler_call_helper::CallParams<decltype(handler)>{
                handler, params, req, asyncResp});
    }
};
} // namespace routing_handler_call_helper
} // namespace detail

class WebSocketRule : public BaseRule
{
    using self_t = WebSocketRule;

  public:
    explicit WebSocketRule(const std::string& ruleIn) : BaseRule(ruleIn)
    {}

    void validate() override
    {}

    void handle(const Request& /*req*/,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const RoutingParams& /*params*/) override
    {
        asyncResp->res.result(boost::beast::http::status::not_found);
    }

    void handleUpgrade(const Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& /*asyncResp*/,
                       boost::asio::ip::tcp::socket&& adaptor) override
    {
        BMCWEB_LOG_DEBUG << "Websocket handles upgrade";
        std::shared_ptr<
            crow::websocket::ConnectionImpl<boost::asio::ip::tcp::socket>>
            myConnection = std::make_shared<
                crow::websocket::ConnectionImpl<boost::asio::ip::tcp::socket>>(
                req, std::move(adaptor), openHandler, messageHandler,
                closeHandler, errorHandler);
        myConnection->start();
    }
#ifdef BMCWEB_ENABLE_SSL
    void handleUpgrade(const Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& /*asyncResp*/,
                       boost::beast::ssl_stream<boost::asio::ip::tcp::socket>&&
                           adaptor) override
    {
        BMCWEB_LOG_DEBUG << "Websocket handles upgrade";
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
    std::function<void(crow::websocket::Connection&)> openHandler;
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

    self_t& name(std::string_view name) noexcept
    {
        self_t* self = static_cast<self_t*>(this);
        self->nameStr = name;
        return *self;
    }

    self_t& methods(boost::beast::http::verb method)
    {
        self_t* self = static_cast<self_t*>(this);
        std::optional<HttpVerb> verb = httpVerbFromBoost(method);
        if (verb)
        {
            self->methodsBitfield = 1U << static_cast<size_t>(*verb);
        }
        return *self;
    }

    template <typename... MethodArgs>
    self_t& methods(boost::beast::http::verb method, MethodArgs... argsMethod)
    {
        self_t* self = static_cast<self_t*>(this);
        methods(argsMethod...);
        std::optional<HttpVerb> verb = httpVerbFromBoost(method);
        if (verb)
        {
            self->methodsBitfield |= 1U << static_cast<size_t>(*verb);
        }
        return *self;
    }

    self_t& notFound()
    {
        self_t* self = static_cast<self_t*>(this);
        self->methodsBitfield = 1U << notFoundIndex;
        return *self;
    }

    self_t& methodNotAllowed()
    {
        self_t* self = static_cast<self_t*>(this);
        self->methodsBitfield = 1U << methodNotAllowedIndex;
        return *self;
    }

    self_t& privileges(
        const std::initializer_list<std::initializer_list<const char*>>& p)
    {
        self_t* self = static_cast<self_t*>(this);
        for (const std::initializer_list<const char*>& privilege : p)
        {
            self->privilegesSet.emplace_back(privilege);
        }
        return *self;
    }

    template <size_t N, typename... MethodArgs>
    self_t& privileges(const std::array<redfish::Privileges, N>& p)
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
    explicit DynamicRule(const std::string& ruleIn) : BaseRule(ruleIn)
    {}

    void validate() override
    {
        if (!erasedHandler)
        {
            throw std::runtime_error(nameStr + (!nameStr.empty() ? ": " : "") +
                                     "no handler for url " + rule);
        }
    }

    void handle(const Request& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const RoutingParams& params) override
    {
        erasedHandler(req, asyncResp, params);
    }

    template <typename Func>
    void operator()(Func f)
    {
        using boost::callable_traits::args_t;
        constexpr size_t arity = std::tuple_size<args_t<Func>>::value;
        constexpr auto is = std::make_integer_sequence<unsigned, arity>{};
        erasedHandler = wrap(std::move(f), is);
    }

    // enable_if Arg1 == request && Arg2 == Response
    // enable_if Arg1 == request && Arg2 != response
    // enable_if Arg1 != request

    template <typename Func, unsigned... Indices>
    std::function<void(const Request&,
                       const std::shared_ptr<bmcweb::AsyncResp>&,
                       const RoutingParams&)>
        wrap(Func f, std::integer_sequence<unsigned, Indices...> /*is*/)
    {
        using function_t = crow::utility::FunctionTraits<Func>;

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
    std::function<void(const Request&,
                       const std::shared_ptr<bmcweb::AsyncResp>&,
                       const RoutingParams&)>
        erasedHandler;
};

template <typename... Args>
class TaggedRule :
    public BaseRule,
    public RuleParameterTraits<TaggedRule<Args...>>
{
  public:
    using self_t = TaggedRule<Args...>;

    explicit TaggedRule(const std::string& ruleIn) : BaseRule(ruleIn)
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

        handler = [f = std::forward<Func>(f)](
                      const Request&,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      Args... args) { asyncResp->res.result(f(args...)); };
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

        handler = [f = std::forward<Func>(f)](
                      const crow::Request& req,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      Args... args) { asyncResp->res.result(f(req, args...)); };
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

        handler = std::forward<Func>(f);
    }

    template <typename Func>
    void operator()(std::string_view name, Func&& f)
    {
        nameStr = name;
        (*this).template operator()<Func>(std::forward(f));
    }

    void handle(const Request& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const RoutingParams& params) override
    {
        detail::routing_handler_call_helper::Call<
            detail::routing_handler_call_helper::CallParams<decltype(handler)>,
            0, 0, 0, 0, black_magic::S<Args...>, black_magic::S<>>()(
            detail::routing_handler_call_helper::CallParams<decltype(handler)>{
                handler, params, req, asyncResp});
    }

  private:
    std::function<void(const crow::Request&,
                       const std::shared_ptr<bmcweb::AsyncResp>&, Args...)>
        handler;
};

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

    Trie() : nodes(1)
    {}

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

    std::pair<unsigned, RoutingParams>
        find(std::string_view reqUrl, const Node* node = nullptr,
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
            if (ret.first != 0U && (found == 0U || found > ret.first))
            {
                found = ret.first;
                matchParams = std::move(ret.second);
            }
        };

        if (node->paramChildrens[static_cast<size_t>(ParamType::INT)] != 0U)
        {
            char c = reqUrl[pos];
            if ((c >= '0' && c <= '9') || c == '+' || c == '-')
            {
                char* eptr = nullptr;
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

        if (node->paramChildrens[static_cast<size_t>(ParamType::UINT)] != 0U)
        {
            char c = reqUrl[pos];
            if ((c >= '0' && c <= '9') || c == '+')
            {
                char* eptr = nullptr;
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

        if (node->paramChildrens[static_cast<size_t>(ParamType::DOUBLE)] != 0U)
        {
            char c = reqUrl[pos];
            if ((c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.')
            {
                char* eptr = nullptr;
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

        if (node->paramChildrens[static_cast<size_t>(ParamType::PATH)] != 0U)
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

        for (const Node::ChildMap::value_type& kv : node->children)
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
        for (size_t i = 0; i < static_cast<size_t>(ParamType::MAX); i++)
        {
            if (n->paramChildrens[i] != 0U)
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
        for (const Node::ChildMap::value_type& kv : n->children)
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
        RoutingParams params;
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
            BMCWEB_LOG_CRITICAL << "Bad index???";
            return route;
        }
        const PerMethod& perMethod = perMethods[index];
        std::pair<unsigned, RoutingParams> found = perMethod.trie.find(url);
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
            FindRoute route =
                findRouteByIndex(req.url().encoded_path(), perMethodIndex);
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

    static void
        isUserPrivileged(const boost::system::error_code& ec, Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         BaseRule& rule, const RoutingParams& params,
                         const dbus::utility::DBusPropertiesMap& userInfoMap)
    {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "GetUserInfo failed...";
            asyncResp->res.result(
                boost::beast::http::status::internal_server_error);
            return;
        }

        std::string userRole{};
        const std::string* userRolePtr = nullptr;
        const bool* remoteUser = nullptr;
        const bool* passwordExpired = nullptr;

        const bool success = sdbusplus::unpackPropertiesNoThrow(
            redfish::dbus_utils::UnpackErrorPrinter(), userInfoMap,
            "UserPrivilege", userRolePtr, "RemoteUser", remoteUser,
            "UserPasswordExpired", passwordExpired);

        if (!success)
        {
            asyncResp->res.result(
                boost::beast::http::status::internal_server_error);
            return;
        }

        if (userRolePtr != nullptr)
        {
            userRole = *userRolePtr;
            BMCWEB_LOG_DEBUG << "userName = " << req.session->username
                             << " userRole = " << *userRolePtr;
        }

        if (remoteUser == nullptr)
        {
            BMCWEB_LOG_ERROR << "RemoteUser property missing or wrong type";
            asyncResp->res.result(
                boost::beast::http::status::internal_server_error);
            return;
        }
        bool expired = false;
        if (passwordExpired == nullptr)
        {
            if (!*remoteUser)
            {
                BMCWEB_LOG_ERROR
                    << "UserPasswordExpired property is expected for"
                       " local user but is missing or wrong type";
                asyncResp->res.result(
                    boost::beast::http::status::internal_server_error);
                return;
            }
        }
        else
        {
            expired = *passwordExpired;
        }

        // Get the user's privileges from the role
        redfish::Privileges userPrivileges =
            redfish::getUserPrivileges(userRole);

        // Set isConfigureSelfOnly based on D-Bus results.  This
        // ignores the results from both pamAuthenticateUser and the
        // value from any previous use of this session.
        req.session->isConfigureSelfOnly = expired;

        // Modify privileges if isConfigureSelfOnly.
        if (req.session->isConfigureSelfOnly)
        {
            // Remove all privileges except ConfigureSelf
            userPrivileges = userPrivileges.intersection(
                redfish::Privileges{"ConfigureSelf"});
            BMCWEB_LOG_DEBUG << "Operation limited to ConfigureSelf";
        }

        if (!rule.checkPrivileges(userPrivileges))
        {
            asyncResp->res.result(boost::beast::http::status::forbidden);
            if (req.session->isConfigureSelfOnly)
            {
                redfish::messages::passwordChangeRequired(
                    asyncResp->res, crow::utility::urlFromPieces(
                                        "redfish", "v1", "AccountService",
                                        "Accounts", req.session->username));
            }
            return;
        }

        req.userRole = userRole;
        rule.handle(req, asyncResp, params);
    }

    template <typename Adaptor>
    void handleUpgrade(const Request& req,
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

        const std::pair<unsigned, RoutingParams>& found =
            trie.find(req.url().encoded_path());
        unsigned ruleIndex = found.first;
        if (ruleIndex == 0U)
        {
            BMCWEB_LOG_DEBUG << "Cannot match rules "
                             << req.url().encoded_path();
            res.result(boost::beast::http::status::not_found);
            return;
        }

        if (ruleIndex >= rules.size())
        {
            throw std::runtime_error("Trie internal structure corrupted!");
        }

        if ((rules[ruleIndex]->getMethods() &
             (1U << static_cast<size_t>(*verb))) == 0)
        {
            BMCWEB_LOG_DEBUG << "Rule found but method mismatch: "
                             << req.url().encoded_path() << " with "
                             << req.methodString() << "("
                             << static_cast<uint32_t>(*verb) << ") / "
                             << rules[ruleIndex]->getMethods();
            asyncResp->res.result(boost::beast::http::status::not_found);
            return;
        }

        BMCWEB_LOG_DEBUG << "Matched rule (upgrade) '" << rules[ruleIndex]->rule
                         << "' " << static_cast<uint32_t>(*verb) << " / "
                         << rules[ruleIndex]->getMethods();

        // any uncaught exceptions become 500s
        try
        {
            rules[ruleIndex]->handleUpgrade(req, asyncResp,
                                            std::forward<Adaptor>(adaptor));
        }
        catch (const std::exception& e)
        {
            BMCWEB_LOG_ERROR << "An uncaught exception occurred: " << e.what();
            asyncResp->res.result(
                boost::beast::http::status::internal_server_error);
            return;
        }
        catch (...)
        {
            BMCWEB_LOG_ERROR
                << "An uncaught exception occurred. The type was unknown "
                   "so no information was available.";
            asyncResp->res.result(
                boost::beast::http::status::internal_server_error);
            return;
        }
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
                foundRoute.route =
                    findRouteByIndex(req.url().encoded_path(), notFoundIndex);
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
        RoutingParams params = std::move(foundRoute.route.params);

        BMCWEB_LOG_DEBUG << "Matched rule '" << rule.rule << "' "
                         << static_cast<uint32_t>(*verb) << " / "
                         << rule.getMethods();

        if (req.session == nullptr)
        {
            rule.handle(req, asyncResp, params);
            return;
        }
        std::string username = req.session->username;

        crow::connections::systemBus->async_method_call(
            [req{std::move(req)}, asyncResp, &rule, params](

                const boost::system::error_code& ec,
                const dbus::utility::DBusPropertiesMap& userInfoMap

                ) mutable {
            isUserPrivileged(ec, req, asyncResp, rule, params, userInfoMap);
            },
            "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
            "xyz.openbmc_project.User.Manager", "GetUserInfo", username);
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
        // rule index 0 has special meaning; preallocate it to avoid
        // duplication.
        PerMethod() : rules(1)
        {}
    };

    std::array<PerMethod, methodNotAllowedIndex + 1> perMethods;
    std::vector<std::unique_ptr<BaseRule>> allRules;
};
} // namespace crow
