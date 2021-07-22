#pragma once

#include <zlib.h>

#include <array>
#include <filesystem>
#include <vector>

class GzFileReader
{
  public:
    bool gzGetLines(std::vector<std::string>& logEntries,
                    const std::string& filename)
    {
        gzFile logStream = gzopen(filename.c_str(), "r");
        if (!logStream)
        {
            BMCWEB_LOG_ERROR << "Can't open gz file: " << filename << '\n';
            return false;
        }

        std::string wholeFile;
        if (!readFile(wholeFile, logStream))
        {
            gzclose(logStream);
            return false;
        }
        std::string newLastMessage;
        std::vector<std::string> parseLogs =
            hostLogEntryParser(wholeFile, newLastMessage);

        // If file doesn't contain any "\r" or "\n", parseLogs should be empty.
        if (parseLogs.empty())
        {
            lastMessage += newLastMessage;
        }
        else
        {
            if (!lastMessage.empty())
            {
                parseLogs.front() = lastMessage + parseLogs.front();
                lastMessage.clear();
            }
            if (!newLastMessage.empty())
            {
                lastMessage = newLastMessage;
            }
            logEntries.insert(logEntries.end(),
                              std::make_move_iterator(parseLogs.begin()),
                              std::make_move_iterator(parseLogs.end()));
        }
        gzclose(logStream);
        return true;
    }

    std::string getLastMessage()
    {
        return lastMessage;
    }

  private:
    std::string lastMessage;
    std::string lastDelimiter;
    size_t totalFilesSize = 0;

    void printErrorMessage(gzFile logStream)
    {
        int errNum = 0;
        const char* errMsg = gzerror(logStream, &errNum);

        BMCWEB_LOG_ERROR << "Error reading gz compressed data.\n"
                         << "Error Message: " << errMsg << '\n'
                         << "Error Number: " << errNum;
    }

    bool readFile(std::string& wholeFile, gzFile logStream)
    {
        // Assume we have 8 files, and the max size of each file is
        // 16k, so define the max size as 256kb (double of 8 files *
        // 16kb)
        constexpr size_t maxTotalFilesSize = 262144;
        constexpr int bufferLimitSize = 1024;
        do
        {
            std::string bufferStr;
            bufferStr.resize(bufferLimitSize);

            int bytesRead = gzread(logStream, bufferStr.data(),
                                   static_cast<unsigned int>(bufferStr.size()));
            // On errors, gzread() shall return a value less than 0.
            if (bytesRead < 0)
            {
                printErrorMessage(logStream);
                return false;
            }
            bufferStr.resize(static_cast<size_t>(bytesRead));
            totalFilesSize += bufferStr.size();
            if (totalFilesSize > maxTotalFilesSize)
            {
                BMCWEB_LOG_ERROR << "File size exceeds maximum allowed size of "
                                 << maxTotalFilesSize;
                return false;
            }
            wholeFile += bufferStr;
        } while (!gzeof(logStream));

        return true;
    }

    std::vector<std::string> hostLogEntryParser(const std::string& wholeFile,
                                                std::string& newLastMessage)
    {
        std::vector<std::string> logEntries;

        // It may contain several log entry in one line, and
        // the end of each log entry will be '\r\n' or '\r'.
        // So we need to go through and split string by '\n' and '\r'
        size_t pos = wholeFile.find_first_of("\n\r");
        size_t initialPos = 0;

        while (pos != std::string::npos)
        {
            std::string logEntry =
                wholeFile.substr(initialPos, pos - initialPos);
            // Since there might be consecutive delimiters like "\r\n", we need
            // to filter empty strings.
            if (!logEntry.empty())
            {
                logEntries.push_back(logEntry);
            }
            else
            {
                // Handle consecutive delimiter. '\r\n' act as a single
                // delimiter, the other case like '\n\n', '\n\r' or '\r\r' will
                // push back a "\n" as a log.
                std::string delimiters;
                if (pos > 0)
                {
                    delimiters = wholeFile.substr(pos - 1, 2);
                }
                // Handle consecutive delimiter but spilt between two files.
                if (pos == 0 && !(lastDelimiter.empty()))
                {
                    delimiters = lastDelimiter + wholeFile.substr(0, 1);
                }
                if (delimiters != "\r\n")
                {
                    logEntries.emplace_back("\n");
                }
            }
            initialPos = pos + 1;
            pos = wholeFile.find_first_of("\n\r", initialPos);
        }

        // Store the last message
        if (initialPos < wholeFile.size())
        {
            newLastMessage = wholeFile.substr(initialPos);
        }
        // If consecutive delimiter spilt by file, the last character of the
        // file must be the delimiter.
        else if (initialPos == wholeFile.size())
        {
            lastDelimiter = wholeFile.substr(initialPos - 1, 1);
        }
        return logEntries;
    }

  public:
    GzFileReader() = default;
    ~GzFileReader() = default;
    GzFileReader(const GzFileReader&) = delete;
    GzFileReader& operator=(const GzFileReader&) = delete;
};
