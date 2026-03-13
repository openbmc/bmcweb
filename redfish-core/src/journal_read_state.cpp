#include "utils/journal_read_state.hpp"

#include "logging.hpp"

#include <systemd/sd-journal.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace redfish
{

JournalReadState::JournalReadState() = default;

std::optional<JournalReadState> JournalReadState::openDefaults()
{
    JournalReadState state;
    int ret = sd_journal_open(&state.journal, SD_JOURNAL_LOCAL_ONLY);
    if (ret < 0)
    {
        BMCWEB_LOG_ERROR("failed to open journal: {}", ret);
        return std::nullopt;
    }
    return state;
}

std::optional<JournalReadState> JournalReadState::openFile(
    const std::string& path)
{
    JournalReadState state;
    std::array<const char*, 2> paths{path.c_str(), nullptr};
    int ret = sd_journal_open_files(&state.journal, paths.data(), 0);
    if (ret < 0)
    {
        BMCWEB_LOG_ERROR("failed to open journal file: {} {}", path, ret);
        return std::nullopt;
    }
    return state;
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

int JournalReadState::seekCursor(const std::string& cursor) const
{
    return sd_journal_seek_cursor(journal, cursor.c_str());
}
int JournalReadState::testCursor(const std::string& cursor) const
{
    return sd_journal_test_cursor(journal, cursor.c_str());
}
std::string JournalReadState::getCursor() const
{
    char* cursor = nullptr;
    if (sd_journal_get_cursor(journal, &cursor) < 0)
    {
        return "";
    }
    if (cursor == nullptr)
    {
        return "";
    }
    std::unique_ptr<char, decltype(&free)> cursorPtr(cursor, &free);
    return {cursorPtr.get()};
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

std::string JournalReadState::getData(const char* field) const
{
    const void* dataVoid = nullptr;
    size_t length = 0;
    int ret = sd_journal_get_data(journal, field, &dataVoid, &length);
    if (ret < 0)
    {
        BMCWEB_LOG_ERROR("failed to get data for field {}: {}", field, ret);
        return {};
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    std::string_view data(reinterpret_cast<const char*>(dataVoid), length);
    std::string_view fieldView(field);
    if (data.size() < fieldView.size() + 1)
    {
        return {};
    }
    if (data.starts_with(fieldView) && data[fieldView.size()] == '=')
    {
        data.remove_prefix(fieldView.size() + 1);
    }

    return std::string{data};
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
