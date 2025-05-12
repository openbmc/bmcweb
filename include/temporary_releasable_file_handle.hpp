#pragma once

#include <boost/beast/core/file_posix.hpp>

#include <filesystem>
#include <string>
#include <string_view>

// Class that represents a file that is allocated in a temporary location.
class TemporaryReleasableFileHandle
{
  public:
    boost::beast::file_posix file;

    TemporaryReleasableFileHandle()
    {
        constexpr std::string_view baseDir{"/tmp/images/tmp_location"};
        std::error_code ec;
        std::filesystem::create_directories(baseDir, ec);
        if (ec)
        {
            BMCWEB_LOG_ERROR("Failed to create {}: {}", baseDir, ec.message());
            return;
        }

        std::filesystem::path tmpl = std::filesystem::path(baseDir) /
                                     "bmcweb_multipart_payload_XXXXXXXXXXX";
        // mkstemp needs a writable C-string.
        std::string tmplStr = tmpl.string();
        int fd = mkstemp(tmplStr.data());
        if (fd == -1)
        {
            BMCWEB_LOG_ERROR("mkstemp({}) failed: {}", tmplStr,
                             strerror(errno));
            return;
        }

        file.native_handle(fd);
        path = tmplStr;
    }

    bool releaseToPath(const std::filesystem::path& destination)
    {
        closeFile();
        std::error_code ec;
        std::filesystem::rename(path, destination, ec);
        if (ec == std::errc::cross_device_link)
        {
            /* different filesystems – copy then remove the original */
            ec.clear();
            std::filesystem::copy_file(
                path, destination,
                std::filesystem::copy_options::overwrite_existing, ec);
            if (!ec)
            {
                std::filesystem::remove(path, ec);
            }
        }
        if (ec)
        {
            BMCWEB_LOG_ERROR("Failed to move file from {} to {}- {}",
                             path.string(), destination.string(), ec.message());
            return false;
        }
        path.clear();
        return true;
    }

    int releaseToFd()
    {
        int fd = dup(file.native_handle());
        closeFile();
        return fd;
    }

    TemporaryReleasableFileHandle(TemporaryReleasableFileHandle&&) = default;
    TemporaryReleasableFileHandle& operator=(TemporaryReleasableFileHandle&&) =
        default;

    TemporaryReleasableFileHandle(const TemporaryReleasableFileHandle& other) :
        path(other.path)
    {
        file.native_handle(dup(other.file.native_handle()));
    }

    TemporaryReleasableFileHandle& operator=(
        const TemporaryReleasableFileHandle& other)
    {
        if (this != &other)
        {
            path = other.path;
            file.native_handle(dup(other.file.native_handle()));
        }
        return *this;
    }

    ~TemporaryReleasableFileHandle()
    {
        closeFile();
        cleanupFile();
    }

  private:
    std::filesystem::path path;

    void closeFile()
    {
        boost::system::error_code ec;
        file.close(ec); // ignore errors – may already be closed
    }

    void cleanupFile()
    {
        if (!path.empty())
        {
            std::error_code ec;
            std::filesystem::remove(path, ec);
            if (ec)
            {
                BMCWEB_LOG_ERROR("Failed to remove temp file {}: {}",
                                 path.string(), ec.message());
            }
            else
            {
                BMCWEB_LOG_DEBUG("Cleaned up {}", path.string());
            }
        }
    }
};
