#include <sdbusplus/asio/connection.hpp>

// helper class to remove const and reference from types
// taken from include/sdbusplus/utility/type_traits.hpp
template <typename T>
struct decay_tuple
{};

template <typename... Args>
struct decay_tuple<std::tuple<Args...>>
{
    using type = std::tuple<typename std::decay<Args>::type...>;
};

class mockConnection : public sdbusplus::asio::connection
{
  public:
    mockConnection()
    {}
    // data owned, deleted, and added by test code
    // queue<void*> unsafe_yield_method_call_return;
    // queue<void*> unsafe_async_method_call_return;
    std::unordered_map<
        std::tuple<std::string, std::string, std::string, std::string>, void*>
        unsafeMap{};
    MOCK_CONST_METHOD3(async_send, message_t& m, MessageHandler&& handler,
                       uint64_t timeout = 0);

    template <typename MessageHandler, typename... InputArgs>
    void async_method_call(MessageHandler&& handler, const std::string& service,
                           const std::string& objpath,
                           const std::string& interf, const std::string& method,
                           uint64_t timeout, const InputArgs&... a)
    {
        // get args
        using FunctionTuple = boost::callable_traits::args_t<
            MessageHandler> using FunctionTupleType =
            utility::decay_tuple_t<FunctionTuple>;

        // strip the first 1 or 2 args
        constexpr bool returnWithMsg = []() {
            if constexpr ((std::tuple_size_v<FunctionTupleType>) > 1)
            {
                return std::is_same_v<
                    std::tuple_element_t<1, FunctionTupleType>,
                    sdbusplus::message_t>;
            }
            return false;
        }();
        using UnpackType = utility::strip_first_n_args_t<returnWithMsg ? 2 : 1,
                                                         FunctionTupleType>;
        // create hanlder // I might not need this because I don't really make a
        // dbus call
        auto applyHandler =
            [handler = std::forward<MessageHandler>(handler)](
                boost::system::error_code ec, message_t& r) mutable {
                UnpackType responseData;
                if (!ec)
                {
                    try
                    {
                        utility::read_into_tuple(responseData, r);
                    }
                    catch (const std::exception& e)
                    {
                        // Set error code if not already set
                        ec = boost::system::errc::make_error_code(
                            boost::system::errc::invalid_argument);
                    }
                }
                // Note.  Callback is called whether or not the unpack was
                // successful to allow the user to implement their own handling
                if constexpr (returnWithMsg)
                {
                    auto response = std::tuple_cat(std::make_tuple(ec),
                                                   std::forward_as_tuple(r),
                                                   std::move(responseData));
                    std::apply(handler, response);
                }
                else
                {
                    auto response = std::tuple_cat(std::make_tuple(ec),
                                                   std::move(responseData));
                    std::apply(handler, response);
                }
            }
        // look up the args in a map, not calling dbus
        message_t m;
        try
        {
            m = unsafeMap[std::tuple(service, objpath, interf, method)];
            m.append(a...);
        }
        catch (...)
        {
            ec = boost::system::errc::make_error_code(
                static_cast<boost::system::errc::errc_t>(e.get_errno()));
        }
        applyHandler(ec, m);

        // run the applyHandler
    }

    template <typename MessageHandler, typename... InputArgs>
    void async_method_call(MessageHandler&& handler, const std::string& service,
                           const std::string& objpath,
                           const std::string& interf, const std::string& method,
                           const InputArgs&... a)
    {
        async_method_call_timed(std::forward<MessageHandler>(handler), service,
                                objpath, interf, method, 0, a...);
    }

    template <typename... RetTypes, typename... InputArgs>
    auto yield_method_call(boost::asio::yield_context yield,
                           boost::system::error_code& ec,
                           const std::string& service,
                           const std::string& objpath,
                           const std::string& interf, const std::string& method,
                           const InputArgs&... a)
    {

        std::tuple<RetTypes...>* responseData =
            reinterpret_cast <
            std::tuple<RetTypes...>(unsafe_yield_method_call_return.front());
        unsafe_yield_method_call_return.pop_front();

        return *responseData;
    }
};
