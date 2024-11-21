#pragma once

#include <sys/inotify.h>

#include <boost/asio/posix/stream_descriptor.hpp>

#include <optional>
#include <string_view>
namespace redfish
{
constexpr const char* eventFormatType = "Event";
constexpr const char* metricReportFormatType = "MetricReport";
constexpr const char* eventServiceFile =
    "/var/lib/bmcweb/eventservice_config.json";

constexpr const char* redfishEventLogDir = "/var/log";
constexpr const char* redfishEventLogFile = "/var/log/redfish";
constexpr const size_t iEventSize = sizeof(inotify_event);

class FilesystemLogWatcher
{
  private:
    int inotifyFd = -1;
    int dirWatchDesc = -1;
    int fileWatchDesc = -1;
    boost::asio::posix::stream_descriptor inotifyConn;
    void watchRedfishEventLogFile();

  public:
    FilesystemLogWatcher(boost::asio::io_context& iocIn);
};
} // namespace redfish
