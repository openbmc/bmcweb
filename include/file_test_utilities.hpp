// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <filesystem>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

struct TemporaryFileHandle
{
    std::filesystem::path path;
    std::string stringPath;

    // Creates a temporary file with the contents provided, removes it on
    // destruction.
    explicit TemporaryFileHandle(std::string_view sampleData) :
        path(std::filesystem::temp_directory_path() /
             "bmcweb_http_response_test_XXXXXXXXXXX")
    {
        stringPath = path.string();

        int fd = mkstemp(stringPath.data());
        EXPECT_GT(fd, 0);
        EXPECT_EQ(write(fd, sampleData.data(), sampleData.size()),
                  sampleData.size());
        close(fd);
    }

    TemporaryFileHandle(const TemporaryFileHandle&) = delete;
    TemporaryFileHandle(TemporaryFileHandle&&) = delete;
    TemporaryFileHandle& operator=(const TemporaryFileHandle&) = delete;
    TemporaryFileHandle& operator=(TemporaryFileHandle&&) = delete;

    ~TemporaryFileHandle()
    {
        std::filesystem::remove(path);
    }
};
