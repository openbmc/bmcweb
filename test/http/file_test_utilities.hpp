#pragma once
#include <filesystem>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

inline std::string makeFile(std::string_view sampleData)
{
    std::filesystem::path path = std::filesystem::temp_directory_path();
    path /= "bmcweb_http_response_test_XXXXXX";
    std::string stringPath = path.string();
    int fd = mkstemp(stringPath.data());
    EXPECT_GT(fd, 0);
    EXPECT_EQ(write(fd, sampleData.data(), sampleData.size()),
              sampleData.size());
    close(fd);
    return stringPath;
}
