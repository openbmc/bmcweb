#include "filter_expr_executor.hpp"
#include "filter_expr_parser_ast.hpp"
#include "filter_expr_printer.hpp"

#include <optional>
#include <string_view>

#include "gmock/gmock.h"

namespace redfish
{

static void filterTrue(std::string_view filterExpr, nlohmann::json json)
{
    std::optional<filter_ast::LogicalAnd> ast = parseFilter(filterExpr);
    EXPECT_TRUE(ast);
    if (!ast)
    {
        return;
    }
    EXPECT_EQ(json["Members"].size(), 1);
    EXPECT_TRUE(applyFilter(json, *ast));
    EXPECT_EQ(json["Members"].size(), 1);
}

static void filterFalse(std::string_view filterExpr, nlohmann::json json)
{
    std::optional<filter_ast::LogicalAnd> ast = parseFilter(filterExpr);
    EXPECT_TRUE(ast);
    if (!ast)
    {
        return;
    }
    EXPECT_EQ(json["Members"].size(), 1);
    EXPECT_TRUE(applyFilter(json, *ast));
    EXPECT_EQ(json["Members"].size(), 0);
}

TEST(FilterParser, Integers)
{
    const nlohmann::json members = R"({"Members": [{"Count": 2}]})"_json;
    // Forward true conditions
    filterTrue("Count eq 2", members);
    filterTrue("Count ne 3", members);
    filterTrue("Count gt 1", members);
    filterTrue("Count ge 2", members);
    filterTrue("Count lt 3", members);
    filterTrue("Count le 2", members);

    // Reverse true conditions
    filterTrue("2 eq Count", members);
    filterTrue("3 ne Count", members);
    filterTrue("3 gt Count", members);
    filterTrue("2 ge Count", members);
    filterTrue("1 lt Count", members);
    filterTrue("2 le Count", members);

    // Forward false conditions
    filterFalse("Count eq 3", members);
    filterFalse("Count ne 2", members);
    filterFalse("Count gt 2", members);
    filterFalse("Count ge 3", members);
    filterFalse("Count lt 2", members);
    filterFalse("Count le 1", members);

    // Reverse false conditions
    filterFalse("3 eq Count", members);
    filterFalse("2 ne Count", members);
    filterFalse("2 gt Count", members);
    filterFalse("1 ge Count", members);
    filterFalse("2 lt Count", members);
    filterFalse("3 le Count", members);
}

TEST(FilterParser, FloatingPointToInteger)
{
    const nlohmann::json members = R"({"Members": [{"Count": 2.0}]})"_json;
    // Forward true conditions
    filterTrue("Count eq 2", members);
    filterTrue("Count ne 3", members);
    filterTrue("Count gt 1", members);
    filterTrue("Count ge 2", members);
    filterTrue("Count lt 3", members);
    filterTrue("Count le 2", members);

    // Reverse true conditions
    filterTrue("2 eq Count", members);
    filterTrue("3 ne Count", members);
    filterTrue("3 gt Count", members);
    filterTrue("2 ge Count", members);
    filterTrue("1 lt Count", members);
    filterTrue("2 le Count", members);

    // Forward false conditions
    filterFalse("Count eq 3", members);
    filterFalse("Count ne 2", members);
    filterFalse("Count gt 2", members);
    filterFalse("Count ge 3", members);
    filterFalse("Count lt 2", members);
    filterFalse("Count le 1", members);

    // Reverse false conditions
    filterFalse("3 eq Count", members);
    filterFalse("2 ne Count", members);
    filterFalse("2 gt Count", members);
    filterFalse("1 ge Count", members);
    filterFalse("2 lt Count", members);
    filterFalse("3 le Count", members);
}

TEST(FilterParser, FloatingPointToFloatingPoint)
{
    const nlohmann::json members = R"({"Members": [{"Count": 2.0}]})"_json;
    // Forward true conditions
    filterTrue("Count eq 2.0", members);
    filterTrue("Count ne 3.0", members);
    filterTrue("Count gt 1.0", members);
    filterTrue("Count ge 2.0", members);
    filterTrue("Count lt 3.0", members);
    filterTrue("Count le 2.0", members);

    // Reverse true conditions
    filterTrue("2.0 eq Count", members);
    filterTrue("3.0 ne Count", members);
    filterTrue("3.0 gt Count", members);
    filterTrue("2.0 ge Count", members);
    filterTrue("1.0 lt Count", members);
    filterTrue("2.0 le Count", members);

    // Forward false conditions
    filterFalse("Count eq 3.0", members);
    filterFalse("Count ne 2.0", members);
    filterFalse("Count gt 2.0", members);
    filterFalse("Count ge 3.0", members);
    filterFalse("Count lt 2.0", members);
    filterFalse("Count le 1.0", members);

    // Reverse false conditions
    filterFalse("3.0 eq Count", members);
    filterFalse("2.0 ne Count", members);
    filterFalse("2.0 gt Count", members);
    filterFalse("1.0 ge Count", members);
    filterFalse("2.0 lt Count", members);
    filterFalse("3.0 le Count", members);
}

TEST(FilterParser, String)
{
    const nlohmann::json members =
        R"({"Members": [{"SerialNumber": "Foo"}]})"_json;
    // Forward true conditions
    filterTrue("SerialNumber eq 'Foo'", members);
    filterTrue("SerialNumber ne 'NotFoo'", members);

    // Reverse true conditions
    filterTrue("'Foo' eq SerialNumber", members);
    filterTrue("'NotFoo' ne SerialNumber", members);

    // Forward false conditions
    filterFalse("SerialNumber eq 'NotFoo'", members);
    filterFalse("SerialNumber ne 'Foo'", members);

    // Reverse false conditions
    filterFalse("'NotFoo' eq SerialNumber", members);
    filterFalse("'Foo' ne SerialNumber", members);
}

} // namespace redfish
