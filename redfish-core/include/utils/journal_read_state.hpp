#include <systemd/sd-journal.h>
#include <optional>
#include <string_view>

namespace redfish
{

struct JournalReadState
{
    JournalReadState();

    int next();
    int next_skip(size_t skip);
    int previous();
    int seek_tail();
    int seek_head();
    int get_cursor(char** cursor);
    int seek_cursor(const char* cursor);
    int test_cursor(const char* cursor);
    std::optional<uint64_t> get_realtime_usec();

    std::string_view get_data(const char* field);

    int get_seqnum(uint64_t& seqnum);

    ~JournalReadState();
    sd_journal* journal = nullptr;
};

}