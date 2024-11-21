#pragma once

#include <sys/inotify.h>

#include <boost/asio/posix/stream_descriptor.hpp>

#include <optional>
#include <string_view>
namespace redfish
{

constexpr const char* redfishEventLogFile = "/var/log/redfish";

class FilesystemLogWatcher
{
  private:
    int inotifyFd = -1;
    int dirWatchDesc = -1;
    int fileWatchDesc = -1;
    boost::asio::posix::stream_descriptor inotifyConn;
    void onINotify(const boost::system::error_code& ec,
                   std::size_t bytesTransferred);
    void watchRedfishEventLogFile();

    std::array<char, 1024> readBuffer;

  public:
    explicit FilesystemLogWatcher(boost::asio::io_context& iocIn);
};
} // namespace redfish
