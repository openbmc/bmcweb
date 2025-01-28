#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

#include <array>
#include <cstddef>
#include <iosfwd>
namespace redfish
{

constexpr const char* redfishEventLogFile = "/var/log/redfish";

class FilesystemLogWatcher
{
  private:
    std::streampos redfishLogFilePosition{0};

    int dirWatchDesc = -1;
    int fileWatchDesc = -1;
    void onINotify(const boost::system::error_code& ec,
                   std::size_t bytesTransferred);

    void resetRedfishFilePosition();

    void watchRedfishEventLogFile();

    void readEventLogsFromFile();

    void cacheRedfishLogFile();

    std::array<char, 1024> readBuffer{};
    // Explicit make the last item so it is canceled before the buffer goes out
    // of scope.
    boost::asio::posix::stream_descriptor inotifyConn;

  public:
    explicit FilesystemLogWatcher(boost::asio::io_context& iocIn);
};
} // namespace redfish
