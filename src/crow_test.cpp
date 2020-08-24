#include "crow.h"

#include <iostream>
#include <sstream>
#include <vector>

#include "gtest/gtest.h"

using namespace std;
using namespace crow;

bool failed__ = false;
void error_print()
{
    cerr << endl;
}

template <typename A, typename... Args>
void error_print(const A& a, Args... args)
{
    cerr << a;
    error_print(args...);
}

template <typename... Args>
void fail(Args... args)
{
    error_print(args...);
    failed__ = true;
}

#define ASSERT_EQUAL(a, b)                                                     \
    if ((a) != (b))                                                            \
    fail(__FILE__ ":", __LINE__, ": Assert fail: expected ", (a), " actual ",  \
         (b), ", " #a " == " #b ", at " __FILE__ ":", __LINE__)
#define ASSERT_NOTEQUAL(a, b)                                                  \
    if ((a) == (b))                                                            \
    fail(__FILE__ ":", __LINE__, ": Assert fail: not expected ", (a),          \
         ", " #a " != " #b ", at " __FILE__ ":", __LINE__)

#define DISABLE_TEST(x)                                                        \
    struct test##x                                                             \
    {                                                                          \
        void test();                                                           \
    } x##_;                                                                    \
    void test##x::test()

#define LOCALHOST_ADDRESS "127.0.0.1"

TEST(Crow, Rule)
{
    TaggedRule<> r("/http/");
    r.name("abc");

    // empty handler - fail to validate
    try
    {
        r.validate();
        fail("empty handler should fail to validate");
    }
    catch (runtime_error& e)
    {}

    int x = 0;

    // registering handler
    r([&x] {
        x = 1;
        return "";
    });

    r.validate();

    Response res;

    // executing handler
    ASSERT_EQUAL(0, x);
    boost::beast::http::request<boost::beast::http::string_body> req{};
    r.handle(Request(req), res, RoutingParams());
    ASSERT_EQUAL(1, x);

    // registering handler with Request argument
    r([&x](const crow::Request&) {
        x = 2;
        return "";
    });

    r.validate();

    // executing handler
    ASSERT_EQUAL(1, x);
    r.handle(Request(req), res, RoutingParams());
    ASSERT_EQUAL(2, x);
}

TEST(Crow, ParameterTagging)
{
    static_assert(black_magic::isValid("<int><int><int>"), "valid url");
    static_assert(!black_magic::isValid("<int><int<<int>"), "invalid url");
    static_assert(!black_magic::isValid("nt>"), "invalid url");
    ASSERT_EQUAL(1, black_magic::getParameterTag("<int>"));
    ASSERT_EQUAL(2, black_magic::getParameterTag("<uint>"));
    ASSERT_EQUAL(3, black_magic::getParameterTag("<float>"));
    ASSERT_EQUAL(3, black_magic::getParameterTag("<double>"));
    ASSERT_EQUAL(4, black_magic::getParameterTag("<str>"));
    ASSERT_EQUAL(4, black_magic::getParameterTag("<string>"));
    ASSERT_EQUAL(5, black_magic::getParameterTag("<path>"));
    ASSERT_EQUAL(6 * 6 + 6 + 1,
                 black_magic::getParameterTag("<int><int><int>"));
    ASSERT_EQUAL(6 * 6 + 6 + 2,
                 black_magic::getParameterTag("<uint><int><int>"));
    ASSERT_EQUAL(6 * 6 + 6 * 3 + 2,
                 black_magic::getParameterTag("<uint><double><int>"));

    // url definition parsed in compile time, build into *one number*, and given
    // to template argument
    static_assert(
        std::is_same<black_magic::S<uint64_t, double, int64_t>,
                     black_magic::Arguments<6 * 6 + 6 * 3 + 2>::type>::value,
        "tag to type container");
}

TEST(Crow, PathRouting)
{
    SimpleApp app;

    BMCWEB_ROUTE(app, "/file")
    ([] { return "file"; });

    BMCWEB_ROUTE(app, "/path/")
    ([] { return "path"; });

    {
        boost::beast::http::request<boost::beast::http::string_body> r{};
        Request req{r};
        Response res;

        req.url = "/file";

        app.handle(req, res);

        ASSERT_EQUAL(200, res.resultInt());
    }
    {
        boost::beast::http::request<boost::beast::http::string_body> r{};
        Request req{r};
        Response res;

        req.url = "/file/";

        app.handle(req, res);
        ASSERT_EQUAL(404, res.resultInt());
    }
    {
        boost::beast::http::request<boost::beast::http::string_body> r{};
        Request req{r};
        Response res;

        req.url = "/path";

        app.handle(req, res);
        ASSERT_NOTEQUAL(404, res.resultInt());
    }
    {
        boost::beast::http::request<boost::beast::http::string_body> r{};
        Request req{r};
        Response res;

        req.url = "/path/";

        app.handle(req, res);
        ASSERT_EQUAL(200, res.resultInt());
    }
}

TEST(Crow, RoutingTest)
{
    SimpleApp app;
    int A{};
    uint32_t b{};
    double C{};
    string D{};
    string E{};

    BMCWEB_ROUTE(app, "/0/<uint>")
    ([&](uint32_t b) {
        b = b;
        return "OK";
    });

    BMCWEB_ROUTE(app, "/1/<int>/<uint>")
    ([&](int a, uint32_t b) {
        A = a;
        b = b;
        return "OK";
    });

    BMCWEB_ROUTE(app, "/4/<int>/<uint>/<double>/<string>")
    ([&](int a, uint32_t b, double c, string d) {
        A = a;
        b = b;
        C = c;
        D = d;
        return "OK";
    });

    BMCWEB_ROUTE(app, "/5/<int>/<uint>/<double>/<string>/<path>")
    ([&](int a, uint32_t b, double c, string d, string e) {
        A = a;
        b = b;
        C = c;
        D = d;
        E = e;
        return "OK";
    });

    app.validate();
    // app.debugPrint();
    {
        boost::beast::http::request<boost::beast::http::string_body> r{};
        Request req{r};
        Response res;

        req.url = "/-1";

        app.handle(req, res);

        ASSERT_EQUAL(404, res.resultInt());
    }

    {
        boost::beast::http::request<boost::beast::http::string_body> r{};
        Request req{r};
        Response res;

        req.url = "/0/1001999";

        app.handle(req, res);

        ASSERT_EQUAL(200, res.resultInt());

        ASSERT_EQUAL(1001999, b);
    }

    {
        boost::beast::http::request<boost::beast::http::string_body> r{};
        Request req{r};
        Response res;

        req.url = "/1/-100/1999";

        app.handle(req, res);

        ASSERT_EQUAL(200, res.resultInt());

        ASSERT_EQUAL(-100, A);
        ASSERT_EQUAL(1999, b);
    }
    {
        boost::beast::http::request<boost::beast::http::string_body> r{};
        Request req{r};
        Response res;

        req.url = "/4/5000/3/-2.71828/hellhere";

        app.handle(req, res);

        ASSERT_EQUAL(200, res.resultInt());

        ASSERT_EQUAL(5000, A);
        ASSERT_EQUAL(3, b);
        ASSERT_EQUAL(-2.71828, C);
        ASSERT_EQUAL("hellhere", D);
    }
    {
        boost::beast::http::request<boost::beast::http::string_body> r{};
        Request req{r};
        Response res;

        req.url = "/5/-5/999/3.141592/hello_there/a/b/c/d";

        app.handle(req, res);

        ASSERT_EQUAL(200, res.resultInt());

        ASSERT_EQUAL(-5, A);
        ASSERT_EQUAL(999, b);
        ASSERT_EQUAL(3.141592, C);
        ASSERT_EQUAL("hello_there", D);
        ASSERT_EQUAL("a/b/c/d", E);
    }
}

TEST(Crow, simple_response_RoutingParams)
{
    ASSERT_EQUAL(100,
                 Response(boost::beast::http::status::continue_).resultInt());
    ASSERT_EQUAL(200, Response("Hello there").resultInt());
    ASSERT_EQUAL(500,
                 Response(boost::beast::http::status::internal_server_error,
                          "Internal Error?")
                     .resultInt());

    RoutingParams rp;
    rp.intParams.push_back(1);
    rp.intParams.push_back(5);
    rp.uintParams.push_back(2);
    rp.doubleParams.push_back(3);
    rp.stringParams.push_back("hello");
    ASSERT_EQUAL(1, rp.get<int64_t>(0));
    ASSERT_EQUAL(5, rp.get<int64_t>(1));
    ASSERT_EQUAL(2, rp.get<uint64_t>(0));
    ASSERT_EQUAL(3, rp.get<double>(0));
    ASSERT_EQUAL("hello", rp.get<string>(0));
}

TEST(Crow, handler_with_response)
{
    SimpleApp app;
    BMCWEB_ROUTE(app, "/")([](const crow::Request&, crow::Response&) {});
}

TEST(Crow, http_method)
{
    SimpleApp app;

    BMCWEB_ROUTE(app, "/").methods(boost::beast::http::verb::post,
                                   boost::beast::http::verb::get)(
        [](const Request& req) {
            if (req.method() == boost::beast::http::verb::get)
                return "2";
            else
                return "1";
        });

    BMCWEB_ROUTE(app, "/get_only")
        .methods(boost::beast::http::verb::get)(
            [](const Request& /*req*/) { return "get"; });
    BMCWEB_ROUTE(app, "/post_only")
        .methods(boost::beast::http::verb::post)(
            [](const Request& /*req*/) { return "post"; });

    // cannot have multiple handlers for the same url
    // BMCWEB_ROUTE(app, "/")
    //.methods(boost::beast::http::verb::get)
    //([]{ return "2"; });

    {
        boost::beast::http::request<boost::beast::http::string_body> r{};
        Request req{r};
        Response res;

        req.url = "/";
        app.handle(req, res);

        ASSERT_EQUAL("2", res.body());
    }
    {
        boost::beast::http::request<boost::beast::http::string_body> r{};
        Request req{r};
        Response res;

        req.url = "/";
        r.method(boost::beast::http::verb::post);
        app.handle(req, res);

        ASSERT_EQUAL("1", res.body());
    }

    {
        boost::beast::http::request<boost::beast::http::string_body> r{};
        Request req{r};
        Response res;

        req.url = "/get_only";
        app.handle(req, res);

        ASSERT_EQUAL("get", res.body());
    }

    {
        boost::beast::http::request<boost::beast::http::string_body> r{};
        Request req{r};
        Response res;

        req.url = "/get_only";
        r.method(boost::beast::http::verb::post);
        app.handle(req, res);

        ASSERT_NOTEQUAL("get", res.body());
    }
}

TEST(Crow, server_handling_error_request)
{
    static char buf[2048];
    SimpleApp app;
    BMCWEB_ROUTE(app, "/")([] { return "A"; });
    Server<SimpleApp> server(&app, LOCALHOST_ADDRESS, 45451);
    auto _ = async(launch::async, [&] { server.run(); });
    std::string sendmsg = "POX";
    asio::io_context is;
    {
        asio::ip::tcp::socket c(is);
        c.connect(asio::ip::tcp::endpoint(
            asio::ip::address::from_string(LOCALHOST_ADDRESS), 45451));

        c.send(asio::buffer(sendmsg));

        try
        {
            c.receive(asio::buffer(buf, 2048));
            fail();
        }
        catch (std::exception& e)
        {
            // std::cerr << e.what() << std::endl;
        }
    }
    server.stop();
}

TEST(Crow, multi_server)
{
    static char buf[2048];
    SimpleApp app1, app2;
    BMCWEB_ROUTE(app1, "/").methods(boost::beast::http::verb::get,
                                    boost::beast::http::verb::post)(
        [] { return "A"; });
    BMCWEB_ROUTE(app2, "/").methods(boost::beast::http::verb::get,
                                    boost::beast::http::verb::post)(
        [] { return "B"; });

    Server<SimpleApp> server1(&app1, LOCALHOST_ADDRESS, 45451);
    Server<SimpleApp> server2(&app2, LOCALHOST_ADDRESS, 45452);

    auto _ = async(launch::async, [&] { server1.run(); });
    auto _2 = async(launch::async, [&] { server2.run(); });

    std::string sendmsg =
        "POST /\r\nContent-Length:3\r\nX-HeaderTest: 123\r\n\r\nA=b\r\n";
    asio::io_context is;
    {
        asio::ip::tcp::socket c(is);
        c.connect(asio::ip::tcp::endpoint(
            asio::ip::address::from_string(LOCALHOST_ADDRESS), 45451));

        c.send(asio::buffer(sendmsg));

        size_t recved = c.receive(asio::buffer(buf, 2048));
        ASSERT_EQUAL('A', buf[recved - 1]);
    }

    {
        asio::ip::tcp::socket c(is);
        c.connect(asio::ip::tcp::endpoint(
            asio::ip::address::from_string(LOCALHOST_ADDRESS), 45452));

        for (auto ch : sendmsg)
        {
            char buf[1] = {ch};
            c.send(asio::buffer(buf));
        }

        size_t recved = c.receive(asio::buffer(buf, 2048));
        ASSERT_EQUAL('b', buf[recved - 1]);
    }

    server1.stop();
    server2.stop();
}

TEST(Crow, black_magic)
{
    using namespace black_magic;
    static_assert(
        std::is_same<void, LastElementType<int, char, void>::type>::value,
        "LastElementType");
    static_assert(
        std::is_same<char, PopBack<int, char,
                                   void>::rebind<LastElementType>::type>::value,
        "pop_back");
    static_assert(
        std::is_same<int, PopBack<int, char, void>::rebind<PopBack>::rebind<
                              LastElementType>::type>::value,
        "pop_back");
}

struct NullMiddleware
{
    struct Context
    {};

    template <typename AllContext>
    void beforeHandle(Request&, Response&, Context&, AllContext&)
    {}

    template <typename AllContext>
    void afterHandle(Request&, Response&, Context&, AllContext&)
    {}
};

struct NullSimpleMiddleware
{
    struct Context
    {};

    void beforeHandle(Request& /*req*/, Response& /*res*/, Context& /*ctx*/)
    {}

    void afterHandle(Request& /*req*/, Response& /*res*/, Context& /*ctx*/)
    {}
};

TEST(Crow, middleware_simple)
{
    App<NullMiddleware, NullSimpleMiddleware> app;
    decltype(app)::server_t server(&app, LOCALHOST_ADDRESS, 45451);
    BMCWEB_ROUTE(app, "/")
    ([&](const crow::Request& req) {
        app.getContext<NullMiddleware>(req);
        app.getContext<NullSimpleMiddleware>(req);
        return "";
    });
}

struct IntSettingMiddleware
{
    struct Context
    {
        int val;
    };

    template <typename AllContext>
    void beforeHandle(Request&, Response&, Context& ctx, AllContext&)
    {
        ctx.val = 1;
    }

    template <typename AllContext>
    void afterHandle(Request&, Response&, Context& ctx, AllContext&)
    {
        ctx.val = 2;
    }
};

std::vector<std::string> test_middleware_context_vector;

struct FirstMW
{
    struct Context
    {
        std::vector<string> v;
    };

    void beforeHandle(Request& /*req*/, Response& /*res*/, Context& ctx)
    {
        ctx.v.push_back("1 before");
    }

    void afterHandle(Request& /*req*/, Response& /*res*/, Context& ctx)
    {
        ctx.v.push_back("1 after");
        test_middleware_context_vector = ctx.v;
    }
};

struct SecondMW
{
    struct Context
    {};
    template <typename AllContext>
    void beforeHandle(Request& req, Response& res, Context&,
                      AllContext& all_ctx)
    {
        all_ctx.template get<FirstMW>().v.push_back("2 before");
        if (req.url == "/break")
            res.end();
    }

    template <typename AllContext>
    void afterHandle(Request&, Response&, Context&, AllContext& all_ctx)
    {
        all_ctx.template get<FirstMW>().v.push_back("2 after");
    }
};

struct ThirdMW
{
    struct Context
    {};
    template <typename AllContext>
    void beforeHandle(Request&, Response&, Context&, AllContext& all_ctx)
    {
        all_ctx.template get<FirstMW>().v.push_back("3 before");
    }

    template <typename AllContext>
    void afterHandle(Request&, Response&, Context&, AllContext& all_ctx)
    {
        all_ctx.template get<FirstMW>().v.push_back("3 after");
    }
};

TEST(Crow, middlewareContext)
{
    static char buf[2048];
    // SecondMW depends on FirstMW (it uses all_ctx.get<FirstMW>)
    // so it leads to compile error if we remove FirstMW from definition
    // App<IntSettingMiddleware, SecondMW> app;
    // or change the order of FirstMW and SecondMW
    // App<IntSettingMiddleware, SecondMW, FirstMW> app;

    App<IntSettingMiddleware, FirstMW, SecondMW, ThirdMW> app;

    int x{};
    BMCWEB_ROUTE(app, "/")
    ([&](const Request& req) {
        {
            auto& ctx = app.getContext<IntSettingMiddleware>(req);
            x = ctx.val;
        }
        {
            auto& ctx = app.getContext<FirstMW>(req);
            ctx.v.push_back("handle");
        }

        return "";
    });
    BMCWEB_ROUTE(app, "/break")
    ([&](const Request& req) {
        {
            auto& ctx = app.getContext<FirstMW>(req);
            ctx.v.push_back("handle");
        }

        return "";
    });

    decltype(app)::server_t server(&app, LOCALHOST_ADDRESS, 45451);
    auto _ = async(launch::async, [&] { server.run(); });
    std::string sendmsg = "GET /\r\n\r\n";
    asio::io_context is;
    {
        asio::ip::tcp::socket c(is);
        c.connect(asio::ip::tcp::endpoint(
            asio::ip::address::from_string(LOCALHOST_ADDRESS), 45451));

        c.send(asio::buffer(sendmsg));

        c.receive(asio::buffer(buf, 2048));
        c.close();
    }
    {
        auto& out = test_middleware_context_vector;
        ASSERT_EQUAL(1, x);
        ASSERT_EQUAL(7, out.size());
        ASSERT_EQUAL("1 before", out[0]);
        ASSERT_EQUAL("2 before", out[1]);
        ASSERT_EQUAL("3 before", out[2]);
        ASSERT_EQUAL("handle", out[3]);
        ASSERT_EQUAL("3 after", out[4]);
        ASSERT_EQUAL("2 after", out[5]);
        ASSERT_EQUAL("1 after", out[6]);
    }
    std::string sendmsg2 = "GET /break\r\n\r\n";
    {
        asio::ip::tcp::socket c(is);
        c.connect(asio::ip::tcp::endpoint(
            asio::ip::address::from_string(LOCALHOST_ADDRESS), 45451));

        c.send(asio::buffer(sendmsg2));

        c.receive(asio::buffer(buf, 2048));
        c.close();
    }
    {
        auto& out = test_middleware_context_vector;
        ASSERT_EQUAL(4, out.size());
        ASSERT_EQUAL("1 before", out[0]);
        ASSERT_EQUAL("2 before", out[1]);
        ASSERT_EQUAL("2 after", out[2]);
        ASSERT_EQUAL("1 after", out[3]);
    }
    server.stop();
}

TEST(Crow, bug_quick_repeated_request)
{
    static char buf[2048];

    SimpleApp app;

    BMCWEB_ROUTE(app, "/")([&] { return "hello"; });

    decltype(app)::server_t server(&app, LOCALHOST_ADDRESS, 45451);
    auto _ = async(launch::async, [&] { server.run(); });
    std::string sendmsg = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    asio::io_context is;
    {
        std::vector<std::future<void>> v;
        for (int i = 0; i < 5; i++)
        {
            v.push_back(async(launch::async, [&] {
                asio::ip::tcp::socket c(is);
                c.connect(asio::ip::tcp::endpoint(
                    asio::ip::address::from_string(LOCALHOST_ADDRESS), 45451));

                for (int j = 0; j < 5; j++)
                {
                    c.send(asio::buffer(sendmsg));

                    size_t received = c.receive(asio::buffer(buf, 2048));
                    ASSERT_EQUAL("hello", std::string(buf + received - 5,
                                                      buf + received));
                }
                c.close();
            }));
        }
    }
    server.stop();
}

TEST(Crow, simple_url_params)
{
    static char buf[2048];

    SimpleApp app;

    QueryString lastUrlParams;

    BMCWEB_ROUTE(app, "/params")
    ([&lastUrlParams](const crow::Request& req) {
        lastUrlParams = std::move(req.urlParams);
        return "OK";
    });

    /// params?h=1&foo=bar&lol&count[]=1&count[]=4&pew=5.2

    decltype(app)::server_t server(&app, LOCALHOST_ADDRESS, 45451);
    auto _ = async(launch::async, [&] { server.run(); });
    asio::io_context is;
    std::string sendmsg;

    // check empty params
    sendmsg = "GET /params\r\n\r\n";
    {
        asio::ip::tcp::socket c(is);
        c.connect(asio::ip::tcp::endpoint(
            asio::ip::address::from_string(LOCALHOST_ADDRESS), 45451));
        c.send(asio::buffer(sendmsg));
        c.receive(asio::buffer(buf, 2048));
        c.close();

        stringstream ss;
        ss << lastUrlParams;

        ASSERT_EQUAL("[  ]", ss.str());
    }
    // check single presence
    sendmsg = "GET /params?foobar\r\n\r\n";
    {
        asio::ip::tcp::socket c(is);
        c.connect(asio::ip::tcp::endpoint(
            asio::ip::address::from_string(LOCALHOST_ADDRESS), 45451));
        c.send(asio::buffer(sendmsg));
        c.receive(asio::buffer(buf, 2048));
        c.close();

        ASSERT_TRUE(lastUrlParams.get("missing") == nullptr);
        ASSERT_TRUE(lastUrlParams.get("foobar") != nullptr);
        ASSERT_TRUE(lastUrlParams.getList("missing").empty());
    }
    // check multiple presence
    sendmsg = "GET /params?foo&bar&baz\r\n\r\n";
    {
        asio::ip::tcp::socket c(is);
        c.connect(asio::ip::tcp::endpoint(
            asio::ip::address::from_string(LOCALHOST_ADDRESS), 45451));
        c.send(asio::buffer(sendmsg));
        c.receive(asio::buffer(buf, 2048));
        c.close();

        ASSERT_TRUE(lastUrlParams.get("missing") == nullptr);
        ASSERT_TRUE(lastUrlParams.get("foo") != nullptr);
        ASSERT_TRUE(lastUrlParams.get("bar") != nullptr);
        ASSERT_TRUE(lastUrlParams.get("baz") != nullptr);
    }
    // check single value
    sendmsg = "GET /params?hello=world\r\n\r\n";
    {
        asio::ip::tcp::socket c(is);
        c.connect(asio::ip::tcp::endpoint(
            asio::ip::address::from_string(LOCALHOST_ADDRESS), 45451));
        c.send(asio::buffer(sendmsg));
        c.receive(asio::buffer(buf, 2048));
        c.close();

        ASSERT_EQUAL(string(lastUrlParams.get("hello")), "world");
    }
    // check multiple value
    sendmsg = "GET /params?hello=world&left=right&up=down\r\n\r\n";
    {
        asio::ip::tcp::socket c(is);
        c.connect(asio::ip::tcp::endpoint(
            asio::ip::address::from_string(LOCALHOST_ADDRESS), 45451));
        c.send(asio::buffer(sendmsg));
        c.receive(asio::buffer(buf, 2048));
        c.close();

        ASSERT_EQUAL(string(lastUrlParams.get("hello")), "world");
        ASSERT_EQUAL(string(lastUrlParams.get("left")), "right");
        ASSERT_EQUAL(string(lastUrlParams.get("up")), "down");
    }
    // check multiple value, multiple types
    sendmsg = "GET /params?int=100&double=123.45&boolean=1\r\n\r\n";
    {
        asio::ip::tcp::socket c(is);
        c.connect(asio::ip::tcp::endpoint(
            asio::ip::address::from_string(LOCALHOST_ADDRESS), 45451));
        c.send(asio::buffer(sendmsg));
        c.receive(asio::buffer(buf, 2048));
        c.close();

        ASSERT_EQUAL(boost::lexical_cast<int>(lastUrlParams.get("int")), 100);
        ASSERT_EQUAL(boost::lexical_cast<double>(lastUrlParams.get("double")),
                     123.45);
        ASSERT_EQUAL(boost::lexical_cast<bool>(lastUrlParams.get("boolean")),
                     true);
    }
    // check single array value
    sendmsg = "GET /params?tmnt[]=leonardo\r\n\r\n";
    {
        asio::ip::tcp::socket c(is);

        c.connect(asio::ip::tcp::endpoint(
            asio::ip::address::from_string(LOCALHOST_ADDRESS), 45451));
        c.send(asio::buffer(sendmsg));
        c.receive(asio::buffer(buf, 2048));
        c.close();

        ASSERT_TRUE(lastUrlParams.get("tmnt") == nullptr);
        ASSERT_EQUAL(lastUrlParams.getList("tmnt").size(), 1);
        ASSERT_EQUAL(string(lastUrlParams.getList("tmnt")[0]), "leonardo");
    }
    // check multiple array value
    sendmsg =
        "GET /params?tmnt[]=leonardo&tmnt[]=donatello&tmnt[]=raphael\r\n\r\n";
    {
        asio::ip::tcp::socket c(is);

        c.connect(asio::ip::tcp::endpoint(
            asio::ip::address::from_string(LOCALHOST_ADDRESS), 45451));
        c.send(asio::buffer(sendmsg));
        c.receive(asio::buffer(buf, 2048));
        c.close();

        ASSERT_EQUAL(lastUrlParams.getList("tmnt").size(), 3);
        ASSERT_EQUAL(string(lastUrlParams.getList("tmnt")[0]), "leonardo");
        ASSERT_EQUAL(string(lastUrlParams.getList("tmnt")[1]), "donatello");
        ASSERT_EQUAL(string(lastUrlParams.getList("tmnt")[2]), "raphael");
    }
    server.stop();
}

TEST(Crow, routeDynamic)
{
    SimpleApp app;
    int x = 1;
    app.routeDynamic("/")([&] {
        x = 2;
        return "";
    });

    app.routeDynamic("/set4")([&](const Request&) {
        x = 4;
        return "";
    });
    app.routeDynamic("/set5")([&](const Request&, Response& res) {
        x = 5;
        res.end();
    });

    app.routeDynamic("/set_int/<int>")([&](int y) {
        x = y;
        return "";
    });

    try
    {
        app.routeDynamic("/invalid_test/<double>/<path>")([]() { return ""; });
        fail();
    }
    catch (std::exception&)
    {}

    // app is in an invalid state when routeDynamic throws an exception.
    try
    {
        app.validate();
        fail();
    }
    catch (std::exception&)
    {}

    {
        boost::beast::http::request<boost::beast::http::string_body> r{};
        Request req{r};
        Response res;
        req.url = "/";
        app.handle(req, res);
        ASSERT_EQUAL(x, 2);
    }
    {
        boost::beast::http::request<boost::beast::http::string_body> r{};
        Request req{r};
        Response res;
        req.url = "/set_int/42";
        app.handle(req, res);
        ASSERT_EQUAL(x, 42);
    }
    {
        boost::beast::http::request<boost::beast::http::string_body> r{};
        Request req{r};
        Response res;
        req.url = "/set5";
        app.handle(req, res);
        ASSERT_EQUAL(x, 5);
    }
    {
        boost::beast::http::request<boost::beast::http::string_body> r{};
        Request req{r};
        Response res;
        req.url = "/set4";
        app.handle(req, res);
        ASSERT_EQUAL(x, 4);
    }
}
