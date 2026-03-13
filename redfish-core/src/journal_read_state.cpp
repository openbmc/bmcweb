#include "utils/journal_read_state.hpp"

#include "logging.hpp"

#include <systemd/sd-journal.h>

#include <optional>
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

int JournalReadState::next()
{
    return sd_journal_next(journal);
}
int JournalReadState::next_skip(size_t skip)
{
    return sd_journal_next_skip(journal, skip);
}
int JournalReadState::previous()
{
    return sd_journal_previous(journal);
}
int JournalReadState::seek_tail()
{
    return sd_journal_seek_tail(journal);
}
int JournalReadState::seek_head()
{
    return sd_journal_seek_head(journal);
}

int JournalReadState::seek_cursor(const char* cursor)
{
    return sd_journal_seek_cursor(journal, cursor);
}
int JournalReadState::test_cursor(const char* cursor)
{
    return sd_journal_test_cursor(journal, cursor);
}
int JournalReadState::get_cursor(char** cursor)
{
    return sd_journal_get_cursor(journal, cursor);
}
std::optional<uint64_t> JournalReadState::get_realtime_usec()
{
    uint64_t timestamp = 0;
    int ret = sd_journal_get_realtime_usec(journal, &timestamp);
    if (ret < 0)
    {
        return std::nullopt;
    }
    return timestamp;
}

std::string_view JournalReadState::get_data(const char* field)
{
    const char* data = nullptr;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const void** dataVoid = reinterpret_cast<const void**>(&data);
    size_t length = 0;
    int ret = sd_journal_get_data(journal, field, dataVoid, nullptr);
    if (ret < 0)
    {
        return std::string_view();
    }
    std::string_view contents(data, length);

    return contents;
}

int JournalReadState::get_seqnum(uint64_t& seqnum)
{
#if LIBSYSTEMD_VERSION >= 254
    return sd_journal_get_seqnum(journal, &seqnum, nullptr);
#else
    return -1;
#endif
}

JournalReadState::~JournalReadState()
{
    if (journal)
    {
        sd_journal_close(journal);
    }
}
} // namespace redfish
