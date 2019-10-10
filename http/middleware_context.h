#pragma once

#include "http_request.h"
#include "http_response.h"
#include "utility.h"

namespace crow
{
namespace detail
{
template <typename... Middlewares>
struct PartialContext
    : public black_magic::PopBack<Middlewares...>::template rebind<
          PartialContext>,
      public black_magic::LastElementType<Middlewares...>::type::Context
{
    using parent_context = typename black_magic::PopBack<
        Middlewares...>::template rebind<::crow::detail::PartialContext>;
    template <size_t N>
    using partial = typename std::conditional<
        N == sizeof...(Middlewares) - 1, PartialContext,
        typename parent_context::template partial<N>>::type;

    template <typename T> typename T::Context& get()
    {
        return static_cast<typename T::Context&>(*this);
    }
};

template <> struct PartialContext<>
{
    template <size_t> using partial = PartialContext;
};

template <size_t N, typename Context, typename Container, typename CurrentMW,
          typename... Middlewares>
bool middlewareCallHelper(Container& middlewares, Request& req, Response& res,
                          Context& ctx);

template <typename... Middlewares>
struct Context : private PartialContext<Middlewares...>
// struct Context : private Middlewares::context... // simple but less type-safe
{
    template <size_t N, typename Context, typename Container>
    friend typename std::enable_if<(N == 0)>::type
        afterHandlersCallHelper(Container& middlewares, Context& ctx,
                                Request& req, Response& res);
    template <size_t N, typename Context, typename Container>
    friend typename std::enable_if<(N > 0)>::type
        afterHandlersCallHelper(Container& middlewares, Context& ctx,
                                Request& req, Response& res);

    template <size_t N, typename Context, typename Container,
              typename CurrentMW, typename... Middlewares2>
    friend bool middlewareCallHelper(Container& middlewares, Request& req,
                                     Response& res, Context& ctx);

    template <typename T> typename T::Context& get()
    {
        return static_cast<typename T::Context&>(*this);
    }

    template <size_t N>
    using partial =
        typename PartialContext<Middlewares...>::template partial<N>;
};
} // namespace detail
} // namespace crow
