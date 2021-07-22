#pragma once

#include <zlib.h>

#include <array>
#include <filesystem>

class GzFile
{
  public:
    GzFile(const std::string& filename) :
        logStream(gzopen(filename.c_str(), "r"))
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
        constexpr int lineLimit = 8192;
        // obmc-console define buffer size as uint8_t buf[4096] to catch one
        // entry, and it may contain several log entry in one line created by
        // phosphor-hostlogger stream mode. So we define 8192 as the buffer
        // limit size to unzip a message.

        buf.clear();
        buf.resize(lineLimit);

        // On failure, gzgets() shall return Z_NULL.
        if (gzgets(logStream, buf.data(), static_cast<int>(buf.size())) ==
            Z_NULL)
        {
            if (!gzeof(logStream))
            {
                int zErrnum = 0;
                const char* errmsg = gzerror(logStream, &zErrnum);

                BMCWEB_LOG_ERROR << "gzGetLine error:\n"
                                 << "Msg: " << errmsg << '\n'
                                 << "Errnum: " << zErrnum;
            }
            return false;
        }
        buf.resize(strlen(buf.data()));
        return true;
    }

  private:
    gzFile logStream;

  public:
    GzFile(const GzFile&) = delete;
    GzFile& operator=(const GzFile&) = delete;
};
