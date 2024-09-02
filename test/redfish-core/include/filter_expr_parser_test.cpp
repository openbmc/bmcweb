#include "filter_expr_parser_ast.hpp"
#include "filter_expr_printer.hpp"

#include <optional>
#include <string_view>

#include <gtest/gtest.h>

namespace redfish
{

static void parse(std::string_view filterExpression, std::string_view outputAst)
{
    std::optional<filter_ast::LogicalAnd> ast = parseFilter(filterExpression);
    FilterExpressionPrinter pr;
    EXPECT_TRUE(ast);
    if (ast)
    {
        EXPECT_EQ(pr(*ast), outputAst);
    }
}

TEST(FilterParser, SpecificationExamples)
{
    parse("ProcessorSummary/Count eq 2",
          "unquoted_string(\"ProcessorSummary/Count\") Equals int(2)");
    parse(
        "ProcessorSummary/Count ge 2",
        "unquoted_string(\"ProcessorSummary/Count\") Greater Than Or Equal int(2)");
    parse("ProcessorSummary/Count gt 2",
          "unquoted_string(\"ProcessorSummary/Count\") Greater Than int(2)");
    parse(
        "MemorySummary/TotalSystemMemoryGiB le 64",
        "unquoted_string(\"MemorySummary/TotalSystemMemoryGiB\") Less Than Or Equal int(64)");
    parse(
        "MemorySummary/TotalSystemMemoryGiB lt 64",
        "unquoted_string(\"MemorySummary/TotalSystemMemoryGiB\") Less Than int(64)");
    parse(
        "SystemType ne 'Physical'",
        R"(unquoted_string("SystemType") Not Equal quoted_string("Physical"))");
    parse(
        "ProcessorSummary/Count eq 2 or ProcessorSummary/Count eq 4",
        R"((unquoted_string("ProcessorSummary/Count") Equals int(2)) or (unquoted_string("ProcessorSummary/Count") Equals int(4)))");
    parse("not ProcessorSummary/Count eq 2",
          "not(unquoted_string(\"ProcessorSummary/Count\") Equals int(2))");
    parse("not ProcessorSummary/Count eq -2",
          "not(unquoted_string(\"ProcessorSummary/Count\") Equals int(-2))");
    parse("Status/State eq 'Enabled')",
          R"(unquoted_string("Status/State") Equals quoted_string("Enabled"))");
    parse(
        "ProcessorSummary/Count eq 2 and MemorySummary/TotalSystemMemoryGiB eq 64",
        R"((unquoted_string("ProcessorSummary/Count") Equals int(2)) and (unquoted_string("MemorySummary/TotalSystemMemoryGiB") Equals int(64)))");
    parse(
        "Status/State eq 'Enabled' and Status/Health eq 'OK' or SystemType eq 'Physical'",
        R"((unquoted_string("Status/State") Equals quoted_string("Enabled")) and ((unquoted_string("Status/Health") Equals quoted_string("OK")) or (unquoted_string("SystemType") Equals quoted_string("Physical"))))");
    parse(
        "(Status/State eq 'Enabled' and Status/Health eq 'OK') or SystemType eq 'Physical'",
        R"(((unquoted_string("Status/State") Equals quoted_string("Enabled")) and (unquoted_string("Status/Health") Equals quoted_string("OK"))) or (unquoted_string("SystemType") Equals quoted_string("Physical")))");
}

TEST(FilterParser, BasicOperations)
{
    // Negation
    EXPECT_TRUE(parseFilter("not(ProcessorSummary/Count eq 2)"));

    // Negative numbers
    EXPECT_TRUE(parseFilter("not(ProcessorSummary/Count eq -2)"));

    // Empty strings
    EXPECT_TRUE(parseFilter("(foo eq '')"));

    // Identity
    EXPECT_TRUE(parseFilter("1 eq 1"));
    EXPECT_TRUE(parseFilter("'foo' eq 'foo'"));

    // Inverted params
    EXPECT_TRUE(parseFilter("2 eq ProcessorSummary/Count"));
    EXPECT_TRUE(parseFilter("'OK' eq Status/Health"));

    // Floating point
    EXPECT_TRUE(parseFilter("Reading eq 4.0"));
    EXPECT_TRUE(parseFilter("Reading eq 1e20"));
    EXPECT_TRUE(parseFilter("Reading eq 1.575E1"));
    EXPECT_TRUE(parseFilter("Reading eq -2.5E-3"));
    EXPECT_TRUE(parseFilter("Reading eq 25E-4"));

    // numeric operators
    EXPECT_TRUE(parseFilter("ProcessorSummary/Count eq 2"));
    EXPECT_TRUE(parseFilter("ProcessorSummary/Count ne 2"));
    EXPECT_TRUE(parseFilter("ProcessorSummary/Count gt 2"));
    EXPECT_TRUE(parseFilter("ProcessorSummary/Count ge 2"));
    EXPECT_TRUE(parseFilter("ProcessorSummary/Count lt 2"));
    EXPECT_TRUE(parseFilter("ProcessorSummary/Count le 2"));

    // String comparison
    EXPECT_TRUE(parseFilter("ProcessorSummary/Count eq 'foo'"));
    EXPECT_TRUE(parseFilter("ProcessorSummary/Count ne 'foo'"));

    // Future, datetime values are strings and need to be compared
    // Make sure they parse
    EXPECT_TRUE(parseFilter("ProcessorSummary/Count lt 'Physical'"));
    EXPECT_TRUE(parseFilter("ProcessorSummary/Count le 'Physical'"));
    EXPECT_TRUE(parseFilter("ProcessorSummary/Count gt 'Physical'"));
    EXPECT_TRUE(parseFilter("ProcessorSummary/Count ge 'Physical'"));
    EXPECT_TRUE(parseFilter("'Physical' lt ProcessorSummary/Count"));
    EXPECT_TRUE(parseFilter("'Physical' le ProcessorSummary/Count"));
    EXPECT_TRUE(parseFilter("'Physical' gt ProcessorSummary/Count"));
    EXPECT_TRUE(parseFilter("'Physical' ge ProcessorSummary/Count"));
}

TEST(FilterParser, Spaces)
{
    // Strings with spaces
    parse("foo eq ' '", R"(unquoted_string("foo") Equals quoted_string(" "))");

    // Lots of spaces between args
    parse("foo       eq       ''",
          R"(unquoted_string("foo") Equals quoted_string(""))");

    // Lots of spaces between parens
    parse("(      foo eq ''      )",
          R"(unquoted_string("foo") Equals quoted_string(""))");

    parse("not           foo eq ''",
          R"(not(unquoted_string("foo") Equals quoted_string("")))");
}

TEST(FilterParser, Failures)
{
    // Invalid expressions
    EXPECT_FALSE(parseFilter("("));
    EXPECT_FALSE(parseFilter(")"));
    EXPECT_FALSE(parseFilter("()"));
    EXPECT_FALSE(parseFilter(""));
    EXPECT_FALSE(parseFilter(" "));
    EXPECT_FALSE(parseFilter("ProcessorSummary/Count eq"));
    EXPECT_FALSE(parseFilter("eq ProcessorSummary/Count"));
    EXPECT_FALSE(parseFilter("not(ProcessorSummary/Count)"));
}
} // namespace redfish
