#pragma once

#include <zlib.h>

constexpr int lineLimit = 8192;
// obmc-console define buffer size as uint8_t buf[4096] to catch one
// message(line) and phosphor-hostlogger will attach addtional infomation like
// timestamp, so we define 8192 as the buffer limit size to unzip a message.

class GzFile
{
  public:
    GzFile(const char* filename)
    {
        logStream = gzopen(filename, "r");
    }

    ~GzFile()
    {
        gzclose(logStream);
    }

    bool gzGetLine(std::string& buf)
    {
        char line[lineLimit] = {0};
        gzgets(logStream, line, lineLimit);
        if (gzeof(logStream))
        {
            return false;
        }
        buf.assign(line);
        return true;
    }

  private:
    gzFile logStream;

  public:
    GzFile(const GzFile&) = delete;
    GzFile& operator=(const GzFile&);
};
