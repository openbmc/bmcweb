#pragma once

// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "logging.hpp"

#include <unistd.h>

#include <boost/beast/core/file_posix.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/system/error_code.hpp>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <variant>
#include <vector>

struct DuplicatableFileHandle
{
    boost::beast::file_posix fileHandle;
    std::filesystem::path tempPath;

    DuplicatableFileHandle() = default;
    DuplicatableFileHandle(DuplicatableFileHandle&&) noexcept = default;
    // Overload copy constructor, because posix doesn't have dup(), but linux
    // does
    DuplicatableFileHandle(const DuplicatableFileHandle& other) :
        tempPath(other.tempPath)
    {
        fileHandle.native_handle(dup(other.fileHandle.native_handle()));
    }
    DuplicatableFileHandle& operator=(const DuplicatableFileHandle& other)
    {
        if (this == &other)
        {
            return *this;
        }
        fileHandle.native_handle(dup(other.fileHandle.native_handle()));
        tempPath = other.tempPath;
        return *this;
    }
    DuplicatableFileHandle& operator=(DuplicatableFileHandle&& other) noexcept =
        default;

    ~DuplicatableFileHandle()
    {
        cleanupTempFile();
    }

    bool createTemporaryFile()
    {
        constexpr std::string_view baseDir{"/tmp/images/tmp_location"};
        std::error_code ec;
        std::filesystem::create_directories(baseDir, ec);
        if (ec)
        {
            BMCWEB_LOG_ERROR("Failed to create {}: {}", baseDir, ec.message());
            return false;
        }

        std::filesystem::path tmpl = std::filesystem::path(baseDir) /
                                     "bmcweb_multipart_payload_XXXXXXXXXXX";
        // mkstemp needs a writable C-string.
        std::string tmplStr = tmpl.string();
        int fd = mkstemp(tmplStr.data());
        if (fd == -1)
        {
            BMCWEB_LOG_ERROR("mkstemp({}) failed: {}", tmplStr, errno);
            return false;
        }

        fileHandle.native_handle(fd);
        tempPath = tmplStr;
        return true;
    }

    bool releaseToPath(const std::filesystem::path& destination)
    {
        if (tempPath.empty())
        {
            return false; // Not a temporary file
        }

        closeFile();
        std::error_code ec;
        std::filesystem::rename(tempPath, destination, ec);
        if (ec == std::errc::cross_device_link)
        {
            /* different filesystems – copy then remove the original */
            ec.clear();
            std::filesystem::copy_file(
                tempPath, destination,
                std::filesystem::copy_options::overwrite_existing, ec);
            if (!ec)
            {
                std::filesystem::remove(tempPath, ec);
            }
        }
        if (ec)
        {
            BMCWEB_LOG_ERROR("Failed to move file from {} to {}- {}",
                             tempPath.string(), destination.string(),
                             ec.message());
            return false;
        }
        tempPath.clear();
        return true;
    }

    void closeFile()
    {
        boost::system::error_code ec;
        fileHandle.close(ec); // ignore errors – may already be closed
    }

    void cleanupTempFile() const
    {
        if (!tempPath.empty())
        {
            std::error_code ec;
            std::filesystem::remove(tempPath, ec);
            if (ec)
            {
                BMCWEB_LOG_ERROR("Failed to remove temp file {}: {}",
                                 tempPath.string(), ec.message());
            }
            else
            {
                BMCWEB_LOG_DEBUG("Cleaned up {}", tempPath.string());
            }
        }
    }
};
