#pragma once
#include "common.hpp"
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include "../redfish-core/include/privileges.hpp"
#include "websocket_class_decl.hpp"
#include "../redfish-core/include/error_messages.hpp"

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

    virtual void handle(const Request&,
                        const std::shared_ptr<bmcweb::AsyncResp>&,
                        const RoutingParams&) = 0;
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

    size_t getMethods();

    bool checkPrivileges(const redfish::Privileges& userPrivileges);

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
        Func f,
        typename std::enable_if<
            !std::is_same<
                typename std::tuple_element<0, std::tuple<Args..., void>>::type,
                const Request&>::value,
            int>::type = 0)
    {
        handler = [f = std::move(f)](
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
        Func f,
        typename std::enable_if<
            std::is_same<
                typename std::tuple_element<0, std::tuple<Args..., void>>::type,
                const Request&>::value &&
                !std::is_same<typename std::tuple_element<
                                  1, std::tuple<Args..., void, void>>::type,
                              const std::shared_ptr<bmcweb::AsyncResp>&>::value,
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
                             const std::shared_ptr<bmcweb::AsyncResp>&>::value,
            int>::type = 0)
    {
        handler = std::move(f);
    }

    template <typename... Args>
    struct HandlerTypeHelper
    {
        using type = std::function<void(
            const crow::Request&, const std::shared_ptr<bmcweb::AsyncResp>&,
            Args...)>;
        using args_type =
            black_magic::S<typename black_magic::PromoteT<Args>...>;
    };

    template <typename... Args>
    struct HandlerTypeHelper<const Request&, Args...>
    {
        using type = std::function<void(
            const crow::Request&, const std::shared_ptr<bmcweb::AsyncResp>&,
            Args...)>;
        using args_type =
            black_magic::S<typename black_magic::PromoteT<Args>...>;
    };

    template <typename... Args>
    struct HandlerTypeHelper<const Request&,
                             const std::shared_ptr<bmcweb::AsyncResp>&, Args...>
    {
        using type = std::function<void(
            const crow::Request&, const std::shared_ptr<bmcweb::AsyncResp>&,
            Args...)>;
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

    void handle(const Request&,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const RoutingParams&) override
    {
        asyncResp->res.result(boost::beast::http::status::not_found);
    }

    void handleUpgrade(const Request& req, Response&,
                       boost::asio::ip::tcp::socket&& adaptor) override;
#ifdef BMCWEB_ENABLE_SSL
    void handleUpgrade(const Request& req, Response&,
                       boost::beast::ssl_stream<boost::asio::ip::tcp::socket>&&
                           adaptor) override;
#endif

    // TODO: Func is a Lambda this function must be #included ?
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
    DynamicRule(const std::string& ruleIn) : BaseRule(ruleIn)
    {}

    void validate() override;

    void handle(const Request& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const RoutingParams& params) override;

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
    std::function<void(const Request&,
                       const std::shared_ptr<bmcweb::AsyncResp>&,
                       const RoutingParams&)>
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

        handler = [f = std::move(f)](
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

        handler = [f = std::move(f)](
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

        handler = std::move(f);
    }

    template <typename Func>
    void operator()(const std::string_view name, Func&& f)
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

        bool isSimpleNode() const;
    };

    Trie();

  private:
    void optimizeNode(Node* node);

    void optimize();

  public:
    void validate();

    void findRouteIndexes(const std::string& reqUrl,
                          std::vector<unsigned>& routeIndexes,
                          const Node* node = nullptr, unsigned pos = 0) const;

    std::pair<unsigned, RoutingParams>
        find(const std::string_view reqUrl, const Node* node = nullptr,
             size_t pos = 0, RoutingParams* params = nullptr) const;

    void add(const std::string& url, unsigned ruleIndex);

  private:
    void debugNodePrint(Node* n, size_t level);

  public:
    void debugPrint();

  private:
    const Node* head() const;

    Node* head();

    unsigned newNode();

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

        if (!ruleIndex)
        {
            // Check to see if this url exists at any verb
            for (const PerMethod& p : perMethods)
            {
                const std::pair<unsigned, RoutingParams>& found2 =
                    p.trie.find(req.url);
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

        if (ruleIndex == ruleSpecialRedirectSlash)
        {
            BMCWEB_LOG_INFO << "Redirecting to a url with trailing slash: "
                            << req.url;
            asyncResp->res.result(
                boost::beast::http::status::moved_permanently);

            // TODO absolute url building
            if (req.getHeaderValue("Host").empty())
            {
                asyncResp->res.addHeader("Location",
                                         std::string(req.url) + "/");
            }
            else
            {
                asyncResp->res.addHeader(
                    "Location", (req.isSecure ? "https://" : "http://") +
                                    std::string(req.getHeaderValue("Host")) +
                                    std::string(req.url) + "/");
            }
            return;
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
            [&req, asyncResp, &rules, ruleIndex, found](
                const boost::system::error_code ec,
                std::map<std::string, std::variant<bool, std::string,
                                                   std::vector<std::string>>>
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
                    asyncResp->res.result(
                        boost::beast::http::status::internal_server_error);
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
                            "/redfish/v1/AccountService/Accounts/" +
                                req.session->username);
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

}