#include "http_request.hpp"
#include "routing.hpp"
#include "utility.hpp"

#include <boost/beast/http/message.hpp> // IWYU pragma: keep
#include <boost/beast/http/verb.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <system_error>

#include <gtest/gtest-message.h>
#include <gtest/gtest-test-part.h>
#include <gtest/gtest.h> // IWYU pragma: keep

namespace bmcweb
{
class AsyncResp;
} // namespace bmcweb

// IWYU pragma: no_include <boost/beast/http/impl/message.hpp>
// IWYU pragma: no_include "gtest/gtest_pred_impl.h"
// IWYU pragma: no_include <boost/intrusive/detail/list_iterator.hpp>

namespace crow
{
namespace
{

using ::crow::black_magic::getParameterTag;

TEST(Router, AllowHeader)
{
    // Callback handler that does nothing
    auto nullCallback = [](const Request&,
                           const std::shared_ptr<bmcweb::AsyncResp>&) {};

    Router router;
    std::error_code ec;

    constexpr const std::string_view url = "/foo";

    Request req{{boost::beast::http::verb::get, url, 11}, ec};

    // No route should return no methods.
    router.validate();
    EXPECT_EQ(router.buildAllowHeader(req), "");

    router.newRuleTagged<getParameterTag(url)>(std::string(url))
        .methods(boost::beast::http::verb::get)(nullCallback);
    router.validate();
    EXPECT_EQ(router.buildAllowHeader(req), "GET");

    router.newRuleTagged<getParameterTag(url)>(std::string(url))
        .methods(boost::beast::http::verb::patch)(nullCallback);
    router.validate();
    EXPECT_EQ(router.buildAllowHeader(req), "GET, PATCH");
}
} // namespace
} // namespace crow