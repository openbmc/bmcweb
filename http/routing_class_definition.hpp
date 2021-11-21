#pragma once
#include "common.hpp"
#include "../redfish-core/include/privileges.hpp"
#include "websocket_class_definition.hpp"

namespace crow
{

class BaseRule
{
public:
    BaseRule(const std::string& thisRule);

    virtual ~BaseRule() = default;

    virtual void validate() = 0;
    std::unique_ptr<BaseRule> upgrade();

    virtual void handle(const Request&,
                        const std::shared_ptr<bmcweb::AsyncResp>&,
                        const RoutingParams&);
    virtual void handleUpgrade(const Request&, Response& res,
                               boost::asio::ip::tcp::socket&&);
#ifdef BMCWEB_ENABLE_SSL
    virtual void
        handleUpgrade(const Request&, Response& res,
                      boost::beast::ssl_stream<boost::asio::ip::tcp::socket>&&);
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

class WebSocketRule : public BaseRule
{
    using self_t = WebSocketRule;

  public:
    WebSocketRule(const std::string& ruleIn);

    void validate() override;

    void handle(const Request&,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const RoutingParams&) override;

    void handleUpgrade(const Request& req, Response&,
                       boost::asio::ip::tcp::socket&& adaptor) override
    ;
#ifdef BMCWEB_ENABLE_SSL
    void handleUpgrade(const Request& req, Response&,
                       boost::beast::ssl_stream<boost::asio::ip::tcp::socket>&&
                           adaptor) override;
#endif

    template <typename Func>
    self_t& onopen(Func f);

    template <typename Func>
    self_t& onmessage(Func f);

    template <typename Func>
    self_t& onclose(Func f);

    template <typename Func>
    self_t& onerror(Func f);

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
    WebSocketRule& websocket();

    self_t& name(const std::string_view name) noexcept;

    self_t& methods(boost::beast::http::verb method);

    template <typename... MethodArgs>
    self_t& methods(boost::beast::http::verb method, MethodArgs... argsMethod);

    self_t& privileges(
        const std::initializer_list<std::initializer_list<const char*>>& p);

    template <size_t N, typename... MethodArgs>
    self_t& privileges(const std::array<redfish::Privileges, N>& p);
};

class DynamicRule : public BaseRule, public RuleParameterTraits<DynamicRule>
{
  public:
    DynamicRule(const std::string& ruleIn);

    void validate() override;

    void handle(const Request& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const RoutingParams& params) override;

    template <typename Func>
    void operator()(Func f);

    // enable_if Arg1 == request && Arg2 == Response
    // enable_if Arg1 == request && Arg2 != response
    // enable_if Arg1 != request

    template <typename Func, unsigned... Indices>
    std::function<void(const Request&,
                       const std::shared_ptr<bmcweb::AsyncResp>&,
                       const RoutingParams&)>
        wrap(Func f, std::integer_sequence<unsigned, Indices...>);

    template <typename Func>
    void operator()(std::string name, Func&& f);

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

    TaggedRule(const std::string& ruleIn);

    void validate() override;

    template <typename Func>
    typename std::enable_if<
        black_magic::CallHelper<Func, black_magic::S<Args...>>::value,
        void>::type
        operator()(Func&& f);

    template <typename Func>
    typename std::enable_if<
        !black_magic::CallHelper<Func, black_magic::S<Args...>>::value &&
            black_magic::CallHelper<
                Func, black_magic::S<crow::Request, Args...>>::value,
        void>::type
        operator()(Func&& f);

    template <typename Func>
    typename std::enable_if<
        !black_magic::CallHelper<Func, black_magic::S<Args...>>::value &&
            !black_magic::CallHelper<
                Func, black_magic::S<crow::Request, Args...>>::value,
        void>::type
        operator()(Func&& f);

    template <typename Func>
    void operator()(const std::string_view name, Func&& f);

    void handle(const Request& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const RoutingParams& params) override;

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
        boost::container::flat_map<std::string, unsigned> children;

        bool isSimpleNode();
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

    DynamicRule& newRuleDynamic(const std::string& rule);

    template <uint64_t N>
    typename black_magic::Arguments<N>::type::template rebind<TaggedRule>&
        newRuleTagged(const std::string& rule);

    void internalAddRuleObject(const std::string& rule, BaseRule* ruleObject);

    void validate();

    template <typename Adaptor>
    void handleUpgrade(const Request& req, Response& res, Adaptor&& adaptor);

    void handle(Request& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp);

    void debugPrint();

    std::vector<const std::string*> getRoutes(const std::string& parent);

  private:
    struct PerMethod
    {
        std::vector<BaseRule*> rules;
        Trie trie;
        // rule index 0, 1 has special meaning; preallocate it to avoid
        // duplication.
        PerMethod();
    };

    const static size_t maxHttpVerbCount =
        static_cast<size_t>(boost::beast::http::verb::unlink);

    std::array<PerMethod, maxHttpVerbCount> perMethods;
    std::vector<std::unique_ptr<BaseRule>> allRules;
};

}