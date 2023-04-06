#pragma once
#include "baserule.hpp"
#include "ruleparametertraits.hpp"
#include "websocket.hpp"

#include <boost/beast/http/verb.hpp>

#include <string>

namespace crow
{
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
    const std::vector<std::string>& params;
    const Request& req;
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp;
};

template <typename F, int NString, typename S1, typename S2>
struct Call
{};

template <typename F, int NString, typename... Args1, typename... Args2>
struct Call<F, NString, black_magic::S<std::string, Args1...>,
            black_magic::S<Args2...>>
{
    void operator()(F cparams)
    {
        using pushed = typename black_magic::S<Args2...>::template push_back<
            CallPair<std::string, NString>>;
        Call<F, NString + 1, black_magic::S<Args1...>, pushed>()(cparams);
    }
};

template <typename F, int NString, typename... Args1>
struct Call<F, NString, black_magic::S<>, black_magic::S<Args1...>>
{
    void operator()(F cparams)
    {
        cparams.handler(cparams.req, cparams.asyncResp,
                        cparams.params[Args1::pos]...);
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
        explicit ReqHandlerWrapper(Func fIn) : f(std::move(fIn)) {}

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
        using args_type = black_magic::S<Args...>;
    };

    template <typename... Args>
    struct HandlerTypeHelper<const Request&, Args...>
    {
        using type = std::function<void(
            const crow::Request& /*req*/,
            const std::shared_ptr<bmcweb::AsyncResp>&, Args...)>;
        using args_type = black_magic::S<Args...>;
    };

    template <typename... Args>
    struct HandlerTypeHelper<const Request&,
                             const std::shared_ptr<bmcweb::AsyncResp>&, Args...>
    {
        using type = std::function<void(
            const crow::Request& /*req*/,
            const std::shared_ptr<bmcweb::AsyncResp>&, Args...)>;
        using args_type = black_magic::S<Args...>;
    };

    typename HandlerTypeHelper<ArgsWrapped...>::type handler;

    void operator()(const Request& req,
                    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                    const std::vector<std::string>& params)
    {
        detail::routing_handler_call_helper::Call<
            detail::routing_handler_call_helper::CallParams<decltype(handler)>,
            0, typename HandlerTypeHelper<ArgsWrapped...>::args_type,
            black_magic::S<>>()(
            detail::routing_handler_call_helper::CallParams<decltype(handler)>{
                handler, params, req, asyncResp});
    }
};
} // namespace routing_handler_call_helper
} // namespace detail

class DynamicRule : public BaseRule, public RuleParameterTraits<DynamicRule>
{
  public:
    explicit DynamicRule(const std::string& ruleIn) : BaseRule(ruleIn) {}

    void validate() override
    {
        if (!erasedHandler)
        {
            throw std::runtime_error("no handler for url " + rule);
        }
    }

    void handle(const Request& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const std::vector<std::string>& params) override
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
                       const std::vector<std::string>&)>
        wrap(Func f, std::integer_sequence<unsigned, Indices...> /*is*/)
    {
        using function_t = crow::utility::FunctionTraits<Func>;

        auto ret = detail::routing_handler_call_helper::Wrapped<
            Func, typename function_t::template arg<Indices>...>();
        ret.template set<typename function_t::template arg<Indices>...>(
            std::move(f));
        return ret;
    }

  private:
    std::function<void(const Request&,
                       const std::shared_ptr<bmcweb::AsyncResp>&,
                       const std::vector<std::string>&)>
        erasedHandler;
};

} // namespace crow
