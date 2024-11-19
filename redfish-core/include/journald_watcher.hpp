#pragma once

#include "error_messages.hpp"
#include "event_service_manager.hpp"
#include "logging.hpp"
#include "utils/journal_utils.hpp"

#include <systemd/sd-journal.h>

#include <boost/asio/posix/stream_descriptor.hpp>

#include <memory>

namespace redfish
{

class JournalWatcher : public std::enable_shared_from_this<JournalWatcher>
{
    sd_journal* journal = nullptr;
    boost::asio::io_context& ioc;
    boost::asio::posix::stream_descriptor stream;
    boost::asio::steady_timer timer;

    std::function<void(nlohmann::json::object_t, std::string_view,
                       std::string_view)>
        handler;

    int eventCount = 0;

  public:
    JournalWatcher(
        boost::asio::io_context& iocIn,
        std::function<void(nlohmann::json::object_t, std::string_view,
                           std::string_view)>&& handlerIn) :
        ioc(iocIn), stream(iocIn), timer(iocIn), handler(handlerIn)
    {
        int ret = sd_journal_open(&journal,
                                  SD_JOURNAL_LOCAL_ONLY | SD_JOURNAL_SYSTEM);
        if (ret < 0)
        {
            BMCWEB_LOG_ERROR("failed to open journal: {}", strerror(-ret));
            return;
        }

        stream.assign(sd_journal_get_fd(journal));
        waitForEvent();
    }

    ~JournalWatcher()
    {
        stream.cancel();
        sd_journal_close(journal);
    }

  private:
    void onFdEvent(const boost::system::error_code& ec)
    {
        if (ec == boost::asio::error::operation_aborted)
        {
            // Shutting down or aborted
            return;
        }
        if (ec)
        {
            BMCWEB_LOG_ERROR("Wait error");
            return;
        }
        int r = sd_journal_process(journal);
        if (r < 0)
        {
            BMCWEB_LOG_ERROR("Can't process journal");
            return;
        }
        switch (r)
        {
            case SD_JOURNAL_NOP:
            {
                //BMCWEB_LOG_DEBUG("sd-journal nop");
            }
            break;
            case SD_JOURNAL_APPEND:
            case SD_JOURNAL_INVALIDATE:
            {
                //BMCWEB_LOG_DEBUG("sd-journal append");
                onAppend();
                return;
            }
            break;
        }

        waitForEvent();
    }

    void formatEvent()
    {
        eventCount++;
        if (eventCount == 100){

        }

        nlohmann::json::object_t eventMessage;
        std::string origin;

        std::string_view syslogID;
        int ret = getJournalMetadata(journal, "SYSLOG_IDENTIFIER", syslogID);
        if (ret < 0)
        {
            BMCWEB_LOG_DEBUG("Failed to read SYSLOG_IDENTIFIER field: {}",
                            strerror(-ret));
            return;
        }
        // Get the severity from the PRIORITY field
        long int severity = 8; // Default to an invalid priority
        ret = getJournalMetadataInt(journal, "PRIORITY", severity);
        if (ret < 0)
        {
            BMCWEB_LOG_DEBUG("Failed to read PRIORITY field: {}", strerror(-ret));
        }

        if (!fillBMCJournalLogEntryJson(journal, eventMessage))
        {
            BMCWEB_LOG_ERROR("Failed to log");
            return;
        }

        // Sending our own journal event causes a storm because eventservice
        // logs N logs per message sent.
        if (!syslogID.starts_with("bmcweb") && severity > 2){
            handler(eventMessage, "", "Event");
        }
    }

    void onSchedule(const std::shared_ptr<JournalWatcher>& /*self*/)
    {
        onAppend();
    }

    void onAppend()
    {
        //BMCWEB_LOG_DEBUG("Journal append handler");
        int ret = sd_journal_next(journal);
        if (ret < 0)
        {
            BMCWEB_LOG_ERROR("sd_journal_get failed {}", ret);
            return;
        }

        if (ret == 0)
        {
            // All done reading, wait for more events
            //BMCWEB_LOG_DEBUG("Last event.  Starting wait.");
            waitForEvent();
        }

        formatEvent();
        // schedule getting the next entry
        boost::asio::post(ioc, std::bind_front(&JournalWatcher::onSchedule,
                                               this, shared_from_this()));
        return;
    }

    void waitForEvent()
    {
        int fd = sd_journal_get_fd(journal);
        if (fd < 0)
        {
            return;
        }
        if (fd != stream.native_handle())
        {
            stream.release();
            stream.assign(fd);
        }
        int events = sd_journal_get_events(journal);
        if (events < 0)
        {
            return;
        }
        if (events & POLLIN)
        {
            stream.async_wait(
                boost::asio::posix::stream_descriptor::wait_read,
                std::bind_front(&JournalWatcher::onFdEvent, this));
        }
        if (events & POLLOUT)
        {
            stream.async_wait(
                boost::asio::posix::stream_descriptor::wait_write,
                std::bind_front(&JournalWatcher::onFdEvent, this));
        }
        if (events & POLLERR)
        {
            stream.async_wait(
                boost::asio::posix::stream_descriptor::wait_error,
                std::bind_front(&JournalWatcher::onFdEvent, this));
        }

        uint64_t timeout = 0;
        int timeRet = sd_journal_get_timeout(journal, &timeout);
        if (timeRet < 0)
        {
            return;
        }
        using clock = std::chrono::steady_clock;

        using SdDuration = std::chrono::duration<uint64_t, std::micro>;
        SdDuration sdTimeout(timeout);
        // sd-bus always returns a 64 bit timeout regardless of architecture,
        // and per the documentation routinely returns UINT64_MAX
        if (sdTimeout > clock::duration::max())
        {
            // No need to start the timer if the expiration is longer than
            // underlying timer can run.
            return;
        }
        auto nativeTimeout = std::chrono::floor<clock::duration>(sdTimeout);
        timer.expires_at(clock::time_point(nativeTimeout));
        timer.async_wait(std::bind_front(&JournalWatcher::onFdEvent, this));
    }
};
} // namespace redfish
