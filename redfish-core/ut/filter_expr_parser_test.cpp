#include "redfish-core/include/filter_expr_parser.hpp"

#include "gmock/gmock.h"

TEST(FilterParser, BasicTypes)
{
    // Basic number types
    EXPECT_TRUE(parseFilterExpression("1"));
    EXPECT_TRUE(parseFilterExpression("10"));

    // Quoted strings
    EXPECT_TRUE(parseFilterExpression("'foo'"));
    EXPECT_TRUE(parseFilterExpression("''"));

    // unquoted strings
    EXPECT_TRUE(parseFilterExpression("Members"));
}

TEST(FilterParser, SpecificationExamples)
{
    // This test lists all of the examples given in section 7.3.4
    EXPECT_TRUE(parseFilterExpression("(Status/State eq 'Enabled')"));

    EXPECT_TRUE(parseFilterExpression(
        "(Status/State eq 'Enabled' and Status/Health eq 'OK') or SystemType eq 'Physical'"));
    EXPECT_TRUE(parseFilterExpression(
        "ProcessorSummary/Count eq 2 and MemorySummary/TotalSystemMemoryGiB gt 64"));
    EXPECT_TRUE(parseFilterExpression("ProcessorSummary/Count eq 2"));
    EXPECT_TRUE(parseFilterExpression("ProcessorSummary/Count ge 2"));
    EXPECT_TRUE(parseFilterExpression("ProcessorSummary/Count gt 2"));
    EXPECT_TRUE(
        parseFilterExpression("MemorySummary/TotalSystemMemoryGiB le 64"));
    EXPECT_TRUE(
        parseFilterExpression("MemorySummary/TotalSystemMemoryGiB lt 64"));
    EXPECT_TRUE(parseFilterExpression("SystemType ne 'Physical'"));
    EXPECT_TRUE(parseFilterExpression("not (ProcessorSummary/Count eq 2)"));
    EXPECT_TRUE(parseFilterExpression(
        "ProcessorSummary/Count eq 2 or ProcessorSummary/Count eq 4"));
}

TEST(FilterParser, BasicOperations)
{
    EXPECT_TRUE(parseFilterExpression("not(ProcessorSummary/Count eq 2)"));
    EXPECT_TRUE(parseFilterExpression("(ProcessorSummary/Count eq 2)"));
    EXPECT_TRUE(parseFilterExpression("(foo eq '')"));
}
