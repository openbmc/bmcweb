#pragma once

#include <zlib.h>

#include <array>
#include <filesystem>
#include <vector>

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

    bool gzGetLines(std::vector<std::string>& logEntries)
    {
        // Log file should leverage by logrotate, and the logrotate
        // setting of file limit size is 16k, so we define file limit
        // size as 20 * 1024 bytes here. But if file content is larger than
        // fileLimitSize, we still need to handle error and message completion
        constexpr int fileLimitSize = 20 * 1024;
        bool messageNotComplete = false;
        std::string lastMessage;

        do
        {
            size_t bytes_read = 0;
            char buffer[fileLimitSize] = "";
            bytes_read =
                static_cast<size_t>(gzread(logStream, buffer, fileLimitSize));

            // If file content is larger than fileLimitSize,
            // "bytes_read < fileLimitSize" means the last round of gzread
            if (bytes_read < fileLimitSize && !gzeof(logStream))
            {
                int zErrnum = 0;
                const char* errmsg = gzerror(logStream, &zErrnum);

                BMCWEB_LOG_ERROR << "gzGetLines error:\n"
                                 << "Msg: " << errmsg << '\n'
                                 << "Errnum: " << zErrnum;
                return false;
            }

            std::string logBuffer(buffer);
            std::string logEntry;
            // It may contain several log entry in one line, and
            // the end of each log entry will be '\r\n' or '\r'.
            // So we need to go through and split string by '\n' and '\r'
            size_t pos = logBuffer.find_first_of("\n\r");
            size_t initialPos = 0;

            while (pos != std::string::npos)
            {
                if (messageNotComplete)
                {
                    logEntry = lastMessage +
                               logBuffer.substr(initialPos, pos - initialPos);
                    messageNotComplete = false;
                }
                else
                {
                    logEntry = logBuffer.substr(initialPos, pos - initialPos);
                }

                if (!logEntry.empty())
                {
                    logEntries.push_back(logEntry);
                }
                initialPos = pos + 1;
                pos = logBuffer.find_first_of("\n\r", initialPos);
            }

            // If bytes_read equal to fileLimitSize and not reach eof, means
            // need to continue reading. Store the message which is not
            // completion.
            if (bytes_read == fileLimitSize && !gzeof(logStream))
            {
                lastMessage =
                    logBuffer.substr(initialPos, bytes_read - initialPos + 1);
                messageNotComplete = true;
            }

        } while (!gzeof(logStream));

        return true;
    }

  private:
    gzFile logStream;

  public:
    GzFile(const GzFile&) = delete;
    GzFile& operator=(const GzFile&) = delete;
};
