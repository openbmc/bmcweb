#include <systemd/sd-journal.h>

#include <optional>
#include <string_view>

namespace redfish
{

struct JournalReadState
{
    JournalReadState();
    explicit JournalReadState(const std::string& path);

    // Non copyable.  Movable
    JournalReadState(const JournalReadState&) = delete;
    JournalReadState(JournalReadState&& other) noexcept : journal(other.journal)
    {
        other.journal = nullptr;
    }
    JournalReadState& operator=(const JournalReadState&) = delete;
    JournalReadState& operator=(JournalReadState&& other) noexcept
    {
        journal = other.journal;
        other.journal = nullptr;
        return *this;
    }

    int next() const;
    int nextSkip(size_t skip) const;
    int previous() const;
    int seekTail() const;
    int seekHead() const;
    std::string getCursor() const;
    int seekCursor(const char* cursor) const;
    int testCursor(const char* cursor) const;
    std::optional<uint64_t> getRealtimeUsec() const;

    std::string_view getData(const char* field) const;

    int getSeqnum(uint64_t& seqnum) const;

    ~JournalReadState();
    sd_journal* journal = nullptr;
};

} // namespace redfish
