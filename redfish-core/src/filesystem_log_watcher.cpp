#include "filesystem_log_watcher.hpp"

#include "event_service_manager.hpp"
#include "logging.hpp"

#include <sys/inotify.h>

#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

#include <array>
#include <cstddef>
#include <cstring>
#include <string>

namespace redfish
{

static constexpr const char* redfishEventLogDir = "/var/log";
static constexpr const size_t iEventSize = sizeof(inotify_event);

void FilesystemLogWatcher::onINotify(const boost::system::error_code& ec,
                                     std::size_t bytesTransferred)
{
    if (ec == boost::asio::error::operation_aborted)
    {
        BMCWEB_LOG_DEBUG("Inotify was canceled (shutdown?)");
        return;
    }
    if (ec)
    {
        BMCWEB_LOG_ERROR("Callback Error: {}", ec.message());
        return;
    }

    BMCWEB_LOG_DEBUG("reading {} via inotify", bytesTransferred);

    std::size_t index = 0;
    while ((index + iEventSize) <= bytesTransferred)
    {
        struct inotify_event event
        {};
        std::memcpy(&event, &readBuffer[index], iEventSize);
        if (event.wd == dirWatchDesc)
        {
            if ((event.len == 0) ||
                (index + iEventSize + event.len > bytesTransferred))
            {
                index += (iEventSize + event.len);
                continue;
            }

            std::string fileName(&readBuffer[index + iEventSize]);
            if (fileName != "redfish")
            {
                index += (iEventSize + event.len);
                continue;
            }

            BMCWEB_LOG_DEBUG("Redfish log file created/deleted. event.name: {}",
                             fileName);
            if (event.mask == IN_CREATE)
            {
                if (fileWatchDesc != -1)
                {
                    BMCWEB_LOG_DEBUG("Remove and Add inotify watcher on "
                                     "redfish event log file");
                    // Remove existing inotify watcher and add
                    // with new redfish event log file.
                    inotify_rm_watch(inotifyFd, fileWatchDesc);
                    fileWatchDesc = -1;
                }

                fileWatchDesc = inotify_add_watch(
                    inotifyFd, redfishEventLogFile, IN_MODIFY);
                if (fileWatchDesc == -1)
                {
                    BMCWEB_LOG_ERROR("inotify_add_watch failed for "
                                     "redfish log file.");
                    return;
                }

                EventServiceManager::getInstance().resetRedfishFilePosition();
                EventServiceManager::getInstance().readEventLogsFromFile();
            }
            else if ((event.mask == IN_DELETE) || (event.mask == IN_MOVED_TO))
            {
                if (fileWatchDesc != -1)
                {
                    inotify_rm_watch(inotifyFd, fileWatchDesc);
                    fileWatchDesc = -1;
                }
            }
        }
        else if (event.wd == fileWatchDesc)
        {
            if (event.mask == IN_MODIFY)
            {
                EventServiceManager::getInstance().readEventLogsFromFile();
            }
        }
        index += (iEventSize + event.len);
    }

    watchRedfishEventLogFile();
}

void FilesystemLogWatcher::watchRedfishEventLogFile()
{
    inotifyConn.async_read_some(
        boost::asio::buffer(readBuffer),
        std::bind_front(&FilesystemLogWatcher::onINotify, this));
}

FilesystemLogWatcher::FilesystemLogWatcher(boost::asio::io_context& ioc) :
    inotifyFd(inotify_init1(IN_NONBLOCK)), inotifyConn(ioc)
{
    BMCWEB_LOG_DEBUG("starting Event Log Monitor");

    if (inotifyFd == -1)
    {
        BMCWEB_LOG_ERROR("inotify_init1 failed.");
        return;
    }

    // Add watch on directory to handle redfish event log file
    // create/delete.
    dirWatchDesc = inotify_add_watch(inotifyFd, redfishEventLogDir,
                                     IN_CREATE | IN_MOVED_TO | IN_DELETE);
    if (dirWatchDesc == -1)
    {
        BMCWEB_LOG_ERROR("inotify_add_watch failed for event log directory.");
        return;
    }

    // Watch redfish event log file for modifications.
    fileWatchDesc =
        inotify_add_watch(inotifyFd, redfishEventLogFile, IN_MODIFY);
    if (fileWatchDesc == -1)
    {
        BMCWEB_LOG_ERROR("inotify_add_watch failed for redfish log file.");
        // Don't return error if file not exist.
        // Watch on directory will handle create/delete of file.
    }

    // monitor redfish event log file
    inotifyConn.assign(inotifyFd);
    watchRedfishEventLogFile();
}
} // namespace redfish
