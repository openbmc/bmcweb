#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

// Forward declare sd_journal from systemd/sd-journal.h so that the header
// symbols don't leak
struct sd_journal;

namespace redfish
{

class JournalReadState
{
  private:
    JournalReadState();

  public:
    static std::optional<JournalReadState> openDefaults();
    static std::optional<JournalReadState> openFile(const std::string& path);

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
    int seekCursor(const std::string& cursor) const;
    int testCursor(const std::string& cursor) const;
    std::optional<uint64_t> getRealtimeUsec() const;

    std::string getData(const char* field) const;

    int getSeqnum(uint64_t& seqnum) const;

    ~JournalReadState();
    sd_journal* journal{nullptr};
};

} // namespace redfish
