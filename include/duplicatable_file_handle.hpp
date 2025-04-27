#pragma once

// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "bmcweb_config.h"

#include "logging.hpp"

#include <unistd.h>

#include <boost/beast/core/file_posix.hpp>

#include <cerrno>
#include <filesystem>
#include <string>
#include <string_view>

struct DuplicatableFileHandle
{
    boost::beast::file_posix fileHandle;
    std::string filePath;

    // Construct from a file descriptor
    explicit DuplicatableFileHandle(int fd)
    {
        fileHandle.native_handle(fd);
    }

    // Creates a temporary file with the contents provided, removes it on
    // destruction.
    explicit DuplicatableFileHandle(std::string_view contents)
    {
        if (!initializeTemporaryFile())
        {
            BMCWEB_LOG_ERROR("Failed to create temporary file");
            return;
        }
        boost::system::error_code ec;
        fileHandle.write(contents.data(), contents.size(), ec);
        if (ec)
        {
            BMCWEB_LOG_ERROR("Failed to write to temporary file: {}",
                             ec.message());
            return;
        }
        fileHandle.close(ec);
        if (ec)
        {
            BMCWEB_LOG_ERROR("Failed to close temporary file: {}",
                             ec.message());
            return;
        }
        BMCWEB_LOG_DEBUG("Created temporary file: {}", filePath);
    }

    static std::optional<DuplicatableFileHandle> createEmptyTemporaryFile()
    {
        DuplicatableFileHandle fh;
        if (!fh.initializeTemporaryFile())
        {
            return std::nullopt;
        }
        return {fh};
    }

    void setFd(int fd)
    {
        fileHandle.native_handle(fd);
    }

    DuplicatableFileHandle() = default;
    DuplicatableFileHandle(DuplicatableFileHandle&&) noexcept = default;
    // Overload copy constructor, because posix doesn't have dup(), but linux
    // does
    DuplicatableFileHandle(const DuplicatableFileHandle& other)
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
        return *this;
    }
    DuplicatableFileHandle& operator=(DuplicatableFileHandle&& other) noexcept =
        default;

    ~DuplicatableFileHandle()
    {
        if (!filePath.empty())
        {
            std::error_code ec;
            std::filesystem::remove(filePath, ec);
            if (ec)
            {
                BMCWEB_LOG_ERROR("Failed to remove temporary file {}: {}",
                                 filePath, ec.value());
            }
        }
    }

    bool moveToPath(const std::filesystem::path& destination)
    {
        if (filePath.empty())
        {
            return false;
        }

        std::error_code ec;
        std::filesystem::rename(filePath, destination, ec);
        if (ec)
        {
            BMCWEB_LOG_ERROR("Failed to move file from {} to {}- {}", filePath,
                             destination.string(), ec.message());
            return false;
        }
        filePath = destination.string();
        return true;
    }

  private:
    bool initializeTemporaryFile()
    {
        std::filesystem::path tempDir("/tmp/bmcweb");
        std::error_code ec;
        std::filesystem::create_directories(tempDir, ec);
        if (ec)
        {
            BMCWEB_LOG_ERROR("Failed to create directory {}: {}",
                             tempDir.string(), ec.value());
            return false;
        }

        filePath = (tempDir / "XXXXXXXXXXX").string();

        int fd = mkstemp(filePath.data());
        if (fd < 0)
        {
            BMCWEB_LOG_ERROR("Failed to create temporary file {}: {}", filePath,
                             errno);
            return false;
        }
        fileHandle.native_handle(fd);

        return true;
    }
};
