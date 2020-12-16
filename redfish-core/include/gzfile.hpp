#pragma once

#include <zlib.h>

#include <array>
#include <filesystem>

constexpr int lineLimit = 8192;
// obmc-console define buffer size as uint8_t buf[4096] to catch one
// message(line) and phosphor-hostlogger will attach addtional infomation like
// timestamp, so we define 8192 as the buffer limit size to unzip a message.

class GzFile
{
  public:
    GzFile(const char* filename) : logStream(gzopen(filename, "r"))
    {
        // If gzopen failed, set Z_NULL to logStream.
        if (!logStream)
        {
            BMCWEB_LOG_ERROR << std::strerror(errno);
            logStream = Z_NULL;
        }
    }

    ~GzFile()
    {
        gzclose(logStream);
    }

    bool gzGetLine(std::string& buf)
    {
        std::array<char, lineLimit> line = {0};

        if (gzeof(logStream))
        {
            return false;
        }
        // On failure, gzgets() shall return Z_NULL.
        if (gzgets(logStream, line.data(), lineLimit) == Z_NULL)
        {
            int zErrnum = 0;
            const char* errmsg = gzerror(logStream, &zErrnum);

            BMCWEB_LOG_ERROR << "gzGetLine error:\n"
                             << "Msg: " << errmsg << '\n'
                             << "Errnum: " << zErrnum;
            return false;
        }

        // The data should contain more than one byte of a line.
        if (strlen(line.data()) >= 1)
        {
            buf.assign(line.data());
            return true;
        }

        return false;
    }

  private:
    gzFile logStream;

  public:
    GzFile(const GzFile&) = delete;
    GzFile& operator=(const GzFile&);
};
