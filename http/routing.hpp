#pragma once

#include "common.hpp"
#include "dbus_utility.hpp"
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
    virtual void handleUpgrade(const Request& /*req*/, Response& res,
                               boost::asio::ip::tcp::socket&& /*adaptor*/)
    {
        res.result(boost::beast::http::status::not_found);
        res.end();
    }
#ifdef BMCWEB_ENABLE_SSL
    virtual void handleUpgrade(
        const Request& /*req*/, Response& res,
        boost::beast::ssl_stream<boost::asio::ip::tcp::socket>&& /*adaptor*/)
    {
        res.result(boost::beast::http::status::not_found);
        res.end();
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
        Func function,
        typename std::enable_if<
            !std::is_same<
                typename std::tuple_element<0, std::tuple<Args..., void>>::type,
                const Request&>::value,
            int>::type /*enable*/
        = 0)
    {
        handler = [function = std::forward<Func>(function)](
                      const Request&,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      Args... args) { asyncResp->res.result(f(args...)); };
    }

    template <typename Req, typename... Args>
    struct ReqHandlerWrapper
    {
        ReqHandlerWrapper(Func fIn) : f(std::move(fIn))
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
        Func function,
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
        handler = ReqHandlerWrapper<Args...>(std::move(function));
        /*handler = (
            [f = std::move(f)]
            (const Request& req, Response& res, Args... args){
                 res.result(f(req, args...));
                 res.end();
            });*/
    }

    template <typename... Args>
    void set(
        Func function,
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
        handler = std::move(function);
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
    WebSocketRule(const std::string& ruleIn) : BaseRule(ruleIn)
    {}

    void validate() override
    {}

    void handle(const Request& /*req*/,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const RoutingParams& /*params*/) override
    {
        asyncResp->res.result(boost::beast::http::status::not_found);
    }

    void handleUpgrade(const Request& req, Response& /*res*/,
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
    void handleUpgrade(const Request& req, Response& /*res*/,
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
    self_t& onopen(Func openCallback)
    {
        openHandler = openCallback;
        return *this;
    }

    template <typename Func>
    self_t& onmessage(Func messageCallback)
    {
        messageHandler = messageCallback;
        return *this;
    }

    template <typename Func>
    self_t& onclose(Func closeCallback)
    {
        closeHandler = closeCallback;
        return *this;
    }

    template <typename Func>
    self_t& onerror(Func errorCallback)
    {
        errorHandler = errorCallback;
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
        WebSocketRule* websockRule = new WebSocketRule(self->rule);
        self->ruleToUpgrade.reset(websockRule);
        return *websockRule;
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

    self_t& privileges(
        const std::initializer_list<std::initializer_list<const char*>>&
            privileges)
    {
        self_t* self = static_cast<self_t*>(this);
        for (const std::initializer_list<const char*>& privilege : privileges)
        {
            self->privilegesSet.emplace_back(privilege);
        }
        return *self;
    }

    template <size_t N, typename... MethodArgs>
    self_t& privileges(const std::array<redfish::Privileges, N>& privileges)
    {
        self_t* self = static_cast<self_t*>(this);
        for (const redfish::Privileges& privilege : privileges)
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

    void handle(const Request& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const RoutingParams& params) override
    {
        erasedHandler(req, asyncResp, params);
    }

    template <typename Func>
    void operator()(Func handleCallback)
    {
        using function_t = utility::function_traits<Func>;
        erasedHandler =
            wrap(std::move(handleCallback),
                 std::make_integer_sequence<unsigned, function_t::arity>{});
    }

    // enable_if Arg1 == request && Arg2 == Response
    // enable_if Arg1 == request && Arg2 != response
    // enable_if Arg1 != request

    template <typename Func, unsigned... Indices>
    std::function<void(const Request&,
                       const std::shared_ptr<bmcweb::AsyncResp>&,
                       const RoutingParams&)>
        /*unused*/ wrap(Func handleCallback,
                        std::integer_sequence<unsigned, Indices...> /*is*/)
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
            std::move(handleCallback));
        return ret;
    }

    template <typename Func>
    void operator()(std::string name, Func&& callbackFunction)
    {
        nameStr = std::move(name);
        (*this).template operator()<Func>(std::forward(callbackFunction));
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
        operator()(Func&& callback)
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

        handler = [callback = std::forward<Func>(callback)](
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
        operator()(Func&& callback)
    {
        static_assert(
            black_magic::CallHelper<Func, black_magic::S<Args...>>::value ||
                black_magic::CallHelper<
                    Func, black_magic::S<crow::Request, Args...>>::value,
            "Handler type is mismatched with URL parameters");
        static_assert(
            !std::is_same<void,
                          decltype(callback(std::declval<crow::Request>(),
                                            std::declval<Args>()...))>::value,
            "Handler function cannot have void return type; valid return "
            "types: "
            "string, int, crow::response,nlohmann::json");

        handler = [callback = std::forward<Func>(callback)](
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
        operator()(Func&& callback)
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
                void, decltype(callback(
                          std::declval<crow::Request>(),
                          std::declval<std::shared_ptr<bmcweb::AsyncResp>&>(),
                          std::declval<Args>()...))>::value,
            "Handler function with response argument should have void "
            "return "
            "type");

        handler = std::forward<Func>(callback);
    }

    template <typename Func>
    void operator()(const std::string_view name, Func&& handleCallback)
    {
        nameStr = name;
        (*this).template operator()<Func>(std::forward(handleCallback));
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
                               [](size_t child) { return child == 0U; });
        }
    };

    Trie() : nodes(1)
    {}

  private:
    void optimizeNode(Node* node)
    {
        for (size_t childIndex : node->paramChildrens)
        {
            if (childIndex == 0U)
            {
                continue;
            }
            Node* child = &nodes[childIndex];
            optimizeNode(child);
        }
        if (node->children.empty())
        {
            return;
        }
        bool mergeWithChild = true;
        for (const Node::ChildMap::value_type& childNode : node->children)
        {
            Node* child = &nodes[childNode.second];
            if (!child->isSimpleNode())
            {
                mergeWithChild = false;
                break;
            }
        }
        if (mergeWithChild)
        {
            Node::ChildMap merged;
            for (const Node::ChildMap::value_type& childNode : node->children)
            {
                Node* child = &nodes[childNode.second];
                for (const Node::ChildMap::value_type& childKv :
                     child->children)
                {
                    merged[childNode.first + childKv.first] = childKv.second;
                }
            }
            node->children = std::move(merged);
            optimizeNode(node);
        }
        else
        {
            for (const Node::ChildMap::value_type& childNode : node->children)
            {
                Node* child = &nodes[childNode.second];
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
        for (const Node::ChildMap::value_type& childIndex : node->children)
        {
            const std::string& fragment = childIndex.first;
            const Node* child = &nodes[childIndex.second];
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
                if (ret.first != 0U && (found == 0U || found > ret.first))
                {
                    found = ret.first;
                    matchParams = std::move(ret.second);
                }
            };

        if (node->paramChildrens[static_cast<size_t>(ParamType::INT)] != 0U)
        {
            char currentChar = reqUrl[pos];
            if ((currentChar >= '0' && currentChar <= '9') ||
                currentChar == '+' || currentChar == '-')
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
            char currentChar = reqUrl[pos];
            if ((currentChar >= '0' && currentChar <= '9') ||
                currentChar == '+')
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
            char currentChar = reqUrl[pos];
            if ((currentChar >= '0' && currentChar <= '9') ||
                currentChar == '+' || currentChar == '-' || currentChar == '.')
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

        for (const Node::ChildMap::value_type& child : node->children)
        {
            const std::string& fragment = child.first;
            const Node* childPtr = &nodes[child.second];

            if (reqUrl.compare(pos, fragment.size(), fragment) == 0)
            {
                std::pair<unsigned, RoutingParams> ret =
                    find(reqUrl, childPtr, pos + fragment.size(), params);
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
            char currentChar = url[i];
            if (currentChar == '<')
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

                for (const std::pair<ParamType, std::string>& param :
                     paramTraits)
                {
                    if (url.compare(i, param.second.size(), param.second) == 0)
                    {
                        size_t index = static_cast<size_t>(param.first);
                        if (nodes[idx].paramChildrens[index] == 0U)
                        {
                            unsigned newNodeIdx = newNode();
                            nodes[idx].paramChildrens[index] = newNodeIdx;
                        }
                        idx = nodes[idx].paramChildrens[index];
                        i += static_cast<unsigned>(param.second.size());
                        break;
                    }
                }

                i--;
            }
            else
            {
                std::string piece(&currentChar, 1);
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
        for (const Node::ChildMap::value_type& childIndex : n->children)
        {
            BMCWEB_LOG_DEBUG
                << std::string(2U * level,
                               ' ') /*<< "(" << childIndex.second << ") "*/
                << childIndex.first;
            debugNodePrint(&nodes[childIndex.second], level + 1);
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
        if (ruleIndex == 0U)
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
            rules[ruleIndex]->handleUpgrade(req, res,
                                            std::forward<Adaptor>(adaptor));
        }
        catch (const std::exception& e)
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

    void handle(Request& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
    {
        if (static_cast<size_t>(req.method()) >= perMethods.size())
        {
            asyncResp->res.result(boost::beast::http::status::not_found);
            return;
        }
        PerMethod& perMethod = perMethods[static_cast<size_t>(req.method())];
        Trie& trie = perMethod.trie;
        std::vector<BaseRule*>& rules = perMethod.rules;

        const std::pair<unsigned, RoutingParams>& found = trie.find(req.url);

        unsigned ruleIndex = found.first;

        if (ruleIndex == 0U)
        {
            // Check to see if this url exists at any verb
            for (const PerMethod& method : perMethods)
            {
                const std::pair<unsigned, RoutingParams>& found2 =
                    method.trie.find(req.url);
                if (found2.first > 0)
                {
                    asyncResp->res.result(
                        boost::beast::http::status::method_not_allowed);
                    return;
                }
            }
            BMCWEB_LOG_DEBUG << "Cannot match rules " << req.url;
            asyncResp->res.result(boost::beast::http::status::not_found);
            return;
        }

        if (ruleIndex >= rules.size())
        {
            throw std::runtime_error("Trie internal structure corrupted!");
        }

        if ((rules[ruleIndex]->getMethods() &
             (1U << static_cast<uint32_t>(req.method()))) == 0)
        {
            BMCWEB_LOG_DEBUG << "Rule found but method mismatch: " << req.url
                             << " with " << req.methodString() << "("
                             << static_cast<uint32_t>(req.method()) << ") / "
                             << rules[ruleIndex]->getMethods();
            asyncResp->res.result(
                boost::beast::http::status::method_not_allowed);
            return;
        }

        BMCWEB_LOG_DEBUG << "Matched rule '" << rules[ruleIndex]->rule << "' "
                         << static_cast<uint32_t>(req.method()) << " / "
                         << rules[ruleIndex]->getMethods();

        if (req.session == nullptr)
        {
            rules[ruleIndex]->handle(req, asyncResp, found.second);
            return;
        }

        crow::connections::systemBus->async_method_call(
            [&req, asyncResp, &rules, ruleIndex,
             found](const boost::system::error_code ec,
                    const std::map<std::string, dbus::utility::DbusVariantType>&
                        userInfo) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "GetUserInfo failed...";
                    asyncResp->res.result(
                        boost::beast::http::status::internal_server_error);
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

                const bool* remoteUserPtr = nullptr;
                auto remoteUserIter = userInfo.find("RemoteUser");
                if (remoteUserIter != userInfo.end())
                {
                    remoteUserPtr = std::get_if<bool>(&remoteUserIter->second);
                }
                if (remoteUserPtr == nullptr)
                {
                    BMCWEB_LOG_ERROR
                        << "RemoteUser property missing or wrong type";
                    asyncResp->res.result(
                        boost::beast::http::status::internal_server_error);
                    return;
                }
                bool remoteUser = *remoteUserPtr;

                bool passwordExpired = false; // default for remote user
                if (!remoteUser)
                {
                    const bool* passwordExpiredPtr = nullptr;
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
                        asyncResp->res.result(
                            boost::beast::http::status::internal_server_error);
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
                    asyncResp->res.result(
                        boost::beast::http::status::forbidden);
                    if (req.session->isConfigureSelfOnly)
                    {
                        redfish::messages::passwordChangeRequired(
                            asyncResp->res,
                            crow::utility::urlFromPieces(
                                "redfish", "v1", "AccountService", "Accounts",
                                req.session->username));
                    }
                    return;
                }

                req.userRole = userRole;
                rules[ruleIndex]->handle(req, asyncResp, found.second);
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

        for (const PerMethod& method : perMethods)
        {
            std::vector<unsigned> routeIndexes;
            method.trie.findRouteIndexes(parent, routeIndexes);
            for (unsigned index : routeIndexes)
            {
                ret.push_back(&method.rules[index]->rule);
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

    const static size_t maxHttpVerbCount =
        static_cast<size_t>(boost::beast::http::verb::unlink);

    std::array<PerMethod, maxHttpVerbCount> perMethods;
    std::vector<std::unique_ptr<BaseRule>> allRules;
};
} // namespace crow
