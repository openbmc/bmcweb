#include "utils/journal_read_state.hpp"

#include "logging.hpp"

#include <systemd/sd-journal.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace redfish
{

JournalReadState::JournalReadState()
{
    int ret = sd_journal_open(&journal, SD_JOURNAL_LOCAL_ONLY);
    if (ret < 0)
    {
        BMCWEB_LOG_ERROR("failed to open journal: {}", ret);
    }
}

JournalReadState::JournalReadState(const std::string& path)
{
    int ret = sd_journal_open_directory(&journal, path.c_str(),
                                        SD_JOURNAL_LOCAL_ONLY);
    if (ret < 0)
    {
        BMCWEB_LOG_ERROR("failed to open journal directory: {}", ret);
    }
}
int JournalReadState::next() const
{
    return sd_journal_next(journal);
}
int JournalReadState::nextSkip(size_t skip) const
{
    return sd_journal_next_skip(journal, skip);
}
int JournalReadState::previous() const
{
    return sd_journal_previous(journal);
}
int JournalReadState::seekTail() const
{
    return sd_journal_seek_tail(journal);
}
int JournalReadState::seekHead() const
{
    return sd_journal_seek_head(journal);
}

int JournalReadState::seekCursor(const char* cursor) const
{
    return sd_journal_seek_cursor(journal, cursor);
}
int JournalReadState::testCursor(const char* cursor) const
{
    return sd_journal_test_cursor(journal, cursor);
}
std::string JournalReadState::getCursor() const
{
    char* cursor = nullptr;
    if (sd_journal_get_cursor(journal, &cursor) < 0)
    {
        return "";
    }
    std::unique_ptr<char, decltype(&free)> cursorPtr(cursor, &free);
    return std::string(cursorPtr.get());
}
std::optional<uint64_t> JournalReadState::getRealtimeUsec() const
{
    uint64_t timestamp = 0;
    int ret = sd_journal_get_realtime_usec(journal, &timestamp);
    if (ret < 0)
    {
        return std::nullopt;
    }
    return timestamp;
}

std::string_view JournalReadState::getData(const char* field) const
{
    const char* data = nullptr;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const void** dataVoid = reinterpret_cast<const void**>(&data);
    size_t length = 0;
    int ret = sd_journal_get_data(journal, field, dataVoid, nullptr);
    if (ret < 0)
    {
        return {};
    }
    std::string_view contents(data, length);

    return contents;
}

int JournalReadState::getSeqnum(uint64_t& seqnum) const
{
#if LIBSYSTEMD_VERSION >= 254
    return sd_journal_get_seqnum(journal, &seqnum, nullptr);
#else
    return -1;
#endif
}

JournalReadState::~JournalReadState()
{
    if (journal != nullptr)
    {
        sd_journal_close(journal);
    }
}
} // namespace redfish
