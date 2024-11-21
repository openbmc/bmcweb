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
#include <optional>
#include <string>

namespace redfish
{
void FilesystemLogWatcher::resetRedfishFilePosition()
{
    // Control would be here when Redfish file is created.
    // Reset File Position as new file is created
    redfishLogFilePosition = 0;
}

void FilesystemLogWatcher::cacheRedfishLogFile()
{
    // Open the redfish file and read till the last record.

    std::ifstream logStream(redfishEventLogFile);
    if (!logStream.good())
    {
        BMCWEB_LOG_ERROR(" Redfish log file open failed ");
        return;
    }
    std::string logEntry;
    while (std::getline(logStream, logEntry))
    {
        redfishLogFilePosition = logStream.tellg();
    }
}

void FilesystemLogWatcher::readEventLogsFromFile()
{
    std::ifstream logStream(redfishEventLogFile);
    if (!logStream.good())
    {
        BMCWEB_LOG_ERROR(" Redfish log file open failed");
        return;
    }

    std::vector<EventLogObjectsType> eventRecords;

    std::string logEntry;

    BMCWEB_LOG_DEBUG("Redfish log file: seek to {}",
                     static_cast<int>(redfishLogFilePosition));

    // Get the read pointer to the next log to be read.
    logStream.seekg(redfishLogFilePosition);

    while (std::getline(logStream, logEntry))
    {
        BMCWEB_LOG_DEBUG("Redfish log file: found new event log entry");
        // Update Pointer position
        redfishLogFilePosition = logStream.tellg();

        std::string idStr;
        if (!event_log::getUniqueEntryID(logEntry, idStr))
        {
            BMCWEB_LOG_DEBUG(
                "Redfish log file: could not get unique entry id for {}",
                logEntry);
            continue;
        }

        std::string timestamp;
        std::string messageID;
        std::vector<std::string> messageArgs;
        if (event_log::getEventLogParams(logEntry, timestamp, messageID,
                                         messageArgs) != 0)
        {
            BMCWEB_LOG_DEBUG("Read eventLog entry params failed for {}",
                             logEntry);
            continue;
        }

        eventRecords.emplace_back(idStr, timestamp, messageID, messageArgs);
    }

    if (eventRecords.empty())
    {
        // No Records to send
        BMCWEB_LOG_DEBUG("No log entries available to be transferred.");
        return;
    }
    EventServiceManager::sendEventsToSubs(eventRecords);
}

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

                resetRedfishFilePosition();
                readEventLogsFromFile();
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
                readEventLogsFromFile();
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

    if (redfishLogFilePosition != 0)
    {
        cacheRedfishLogFile();
    }
}
} // namespace redfish
