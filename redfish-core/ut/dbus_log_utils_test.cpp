/*!
 * @file    dbus_log_utils_test.cpp
 * @brief   Source code for dbus logging utility testing.
 */

/* -------------------------------- Includes -------------------------------- */
#include <utils/dbus_log_utils.hpp>

#include <gmock/gmock.h>

namespace redfish
{

TEST(DbusLogUtilsTest, TranslateSeverityDbusToRedfish)
{
    EXPECT_STREQ(translateSeverityDbusToRedfish(
                     "xyz.openbmc_project.Logging.Entry.Level.Alert")
                     .c_str(),
                 "Critical");
    EXPECT_STREQ(translateSeverityDbusToRedfish(
                     "xyz.openbmc_project.Logging.Entry.Level.Critical")
                     .c_str(),
                 "Critical");
    EXPECT_STREQ(translateSeverityDbusToRedfish(
                     "xyz.openbmc_project.Logging.Entry.Level.Emergency")
                     .c_str(),
                 "Critical");
    EXPECT_STREQ(translateSeverityDbusToRedfish(
                     "xyz.openbmc_project.Logging.Entry.Level.Error")
                     .c_str(),
                 "Critical");
    EXPECT_STREQ(translateSeverityDbusToRedfish(
                     "xyz.openbmc_project.Logging.Entry.Level.Debug")
                     .c_str(),
                 "OK");
    EXPECT_STREQ(translateSeverityDbusToRedfish(
                     "xyz.openbmc_project.Logging.Entry.Level.Informational")
                     .c_str(),
                 "OK");
    EXPECT_STREQ(translateSeverityDbusToRedfish(
                     "xyz.openbmc_project.Logging.Entry.Level.Notice")
                     .c_str(),
                 "OK");
    EXPECT_STREQ(translateSeverityDbusToRedfish(
                     "xyz.openbmc_project.Logging.Entry.Level.Warning")
                     .c_str(),
                 "Warning");
    EXPECT_STREQ(translateSeverityDbusToRedfish(
                     "xyz.openbmc_project.Logging.Entry.Level.Abc")
                     .c_str(),
                 "");
}

TEST(DbusLogUtilsTest, AdditionalData)
{
    AdditionalData data1({"field1=a", "field2=b"});
    EXPECT_STREQ(data1["field1"].c_str(), "a");
    EXPECT_STREQ(data1["field2"].c_str(), "b");
    EXPECT_EQ(data1.count("field1"), 1);
    EXPECT_EQ(data1.count("field2"), 1);
    EXPECT_EQ(data1.count("field"), 0);

    AdditionalData data2({"field1=a", "field2=b", "field1=c"},
                         AdditionalData::SameKeyOp::append);
    EXPECT_STREQ(data2["field1"].c_str(), "a;c");
    EXPECT_STREQ(data2["field2"].c_str(), "b");
}

} // namespace redfish