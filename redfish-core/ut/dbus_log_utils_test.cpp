/*
// Copyright (c) 2021 NVIDIA Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

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
        "xyz.openbmc_project.Logging.Entry.Level.Alert").c_str(), "Critical");
    EXPECT_STREQ(translateSeverityDbusToRedfish(
        "xyz.openbmc_project.Logging.Entry.Level.Critical").c_str(),
        "Critical");
    EXPECT_STREQ(translateSeverityDbusToRedfish(
        "xyz.openbmc_project.Logging.Entry.Level.Emergency").c_str(),
        "Critical");
    EXPECT_STREQ(translateSeverityDbusToRedfish(
        "xyz.openbmc_project.Logging.Entry.Level.Error").c_str(), "Critical");
    EXPECT_STREQ(translateSeverityDbusToRedfish(
        "xyz.openbmc_project.Logging.Entry.Level.Debug").c_str(), "OK");
    EXPECT_STREQ(translateSeverityDbusToRedfish(
        "xyz.openbmc_project.Logging.Entry.Level.Informational").c_str(), "OK");
    EXPECT_STREQ(translateSeverityDbusToRedfish(
        "xyz.openbmc_project.Logging.Entry.Level.Notice").c_str(), "OK");
    EXPECT_STREQ(translateSeverityDbusToRedfish(
        "xyz.openbmc_project.Logging.Entry.Level.Warning").c_str(), "Warning");
    EXPECT_STREQ(translateSeverityDbusToRedfish(
        "xyz.openbmc_project.Logging.Entry.Level.Abc").c_str(), "");
}

TEST(DbusLogUtilsTest, AdditionalData)
{
    AdditionalData data({
        "field1=a",
        "field2=b"});

    EXPECT_STREQ(data["field1"].c_str(), "a");
    EXPECT_STREQ(data["field2"].c_str(), "b");
    EXPECT_EQ(data.count("field1"), 1);
    EXPECT_EQ(data.count("field2"), 1);
    EXPECT_EQ(data.count("field"), 0);
}

} // namespace redfish