#pragma once

#include "external_storer_entry.hpp"

namespace external_storer
{

using SeqNum = int64_t;

class Entry
{
  private:
    SeqNum seq;

  public:
    explicit Entry(SeqNum s) : seq(s)
    {}
    Entry(const Entry& copy) = delete;
    Entry& operator=(const Entry& assign) = delete;
};

} // namespace external_storer
