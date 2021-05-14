#include "nlohmann/json.hpp"
#include "routing.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
using namespace crow;
using namespace std;
TEST(PriorityTest, RuleConstructor)
{
    crow::Router router;
    string s;
    /* Test basic wild card prority */
    {
        constexpr char url1[]{"redfish/1/<path>"};
        constexpr char url2[]{"redfish/1/1"};
        auto& rule1 =
            router.newRuleTagged<crow::black_magic::getParameterTag(url1)>(
                url1);
        rule1.name("rule1").priority(1).methods(boost::beast::http::verb::get)(
            [&](const string&) {
                s = rule1.nameStr;
                return boost::beast::http::status::ok;
            });

        auto& rule2 =
            router.newRuleTagged<crow::black_magic::getParameterTag(url2)>(
                url2);
        rule2.name("rule2").priority(0).methods(boost::beast::http::verb::get)(
            [&]() {
                s = rule2.nameStr;
                return boost::beast::http::status::ok;
            });

        router.validate();

        boost::beast::http::request<boost::beast::http::string_body> r{};
        r.method(boost::beast::http::verb::get);
        Request req{r};

        // directory url
        req.url = "redfish/1/1/";
        Response response;
        auto res = std::make_shared<bmcweb::AsyncResp>(response);
        router.handle(req, res);

        EXPECT_THAT(s, ::testing::Eq("rule1"));
    }

    {
        // directory rule url
        constexpr char url1[]{
            "redfish/2/<int>/<uint>/<double>/<string>/<path>/"};
        constexpr char url2[]{"redfish/2/1/1/1/test/path/to/test/"};
        auto& rule1 =
            router.newRuleTagged<crow::black_magic::getParameterTag(url1)>(
                url1);

        // rule 1 with default priority = 0
        rule1.name("rule1").priority(1).methods(boost::beast::http::verb::get)(
            [&](long, unsigned long, double, const string&, const string&) {
                s = rule1.nameStr;
                return boost::beast::http::status::ok;
            });

        // rule 2 with min priority
        auto& rule2 =
            router.newRuleTagged<crow::black_magic::getParameterTag(url2)>(
                url2);
        rule2.name("rule2")
            .priority(std::decay_t<decltype(rule2)>::priorityMin())
            .methods(boost::beast::http::verb::get)([&]() {
                s = rule2.nameStr;
                return boost::beast::http::status::ok;
            });

        router.validate();

        boost::beast::http::request<boost::beast::http::string_body> r{};
        r.method(boost::beast::http::verb::get);
        Request req{r};
        req.url = "redfish/2/1/1/1/test/path/to/test";
        Response response;
        auto res = std::make_shared<bmcweb::AsyncResp>(response);
        router.handle(req, res);

        EXPECT_THAT(s, ::testing::Eq("rule1"));
    }

    /* testing dynamic rules */
    {
        constexpr char url1[]{"redfish/3/<int>/<uint>/<double>"};
        constexpr char url2[]{"redfish/3/1/1/1"};
        auto& rule1 = router.newRuleDynamic(url1);

        // rule 1 with default priority = 0
        rule1.name("rule1").methods(boost::beast::http::verb::get)(
            [&](long, unsigned long, double) {
                s = rule1.nameStr;
                return boost::beast::http::status::ok;
            });

        // rule 2 with max prioirty
        auto& rule2 = router.newRuleDynamic(url2);
        rule2.name("rule2")
            .priority(rule2.priorityMax())
            .methods(boost::beast::http::verb::get)([&]() {
                s = rule2.nameStr;
                return boost::beast::http::status::ok;
            });

        router.validate();

        boost::beast::http::request<boost::beast::http::string_body> r{};
        r.method(boost::beast::http::verb::get);
        Request req{r};
        req.url = "redfish/3/1/1/1";
        Response response;
        auto res = std::make_shared<bmcweb::AsyncResp>(response);
        router.handle(req, res);

        EXPECT_THAT(s, ::testing::Eq("rule2"));
    }

    // dynamic rule and tagged rule
    {
        constexpr char url1[]{
            "redfish/4/<int>/<uint>/<double>/<string>/<path>"};
        constexpr char url2[]{"redfish/4/1/1/1/test/path/to/test"};
        auto& rule1 =
            router.newRuleTagged<crow::black_magic::getParameterTag(url1)>(
                url1);

        rule1.name("rule1")
            .priority(rule1.priorityMax())
            .methods(boost::beast::http::verb::post)(
                [&](long, unsigned long, double, string, string) {
                    s = rule1.nameStr;
                    return boost::beast::http::status::ok;
                });

        auto& rule2 = router.newRuleDynamic(url2);
        rule2.name("rule2").methods(boost::beast::http::verb::post)([&]() {
            s = rule2.nameStr;
            return boost::beast::http::status::ok;
        });

        router.validate();

        boost::beast::http::request<boost::beast::http::string_body> r{};
        r.method(boost::beast::http::verb::post);
        Request req{r};
        req.url = "redfish/4/1/1/1/test/path/to/test";
        Response response;
        auto res = std::make_shared<bmcweb::AsyncResp>(response);
        router.handle(req, res);

        EXPECT_THAT(s, ::testing::Eq("rule1"));
    }
}
