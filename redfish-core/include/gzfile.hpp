#pragma once

#include "logging.hpp"

#include <zlib.h>

#include <array>
#include <filesystem>
#include <string>
#include <vector>

class GzFileReader
{
  public:
    bool gzGetLines(const std::string& filename, uint64_t skip, uint64_t top,
                    std::vector<std::string>& logEntries, size_t& logCount)
    {
        gzFile logStream = gzopen(filename.c_str(), "r");
        if (logStream == nullptr)
        {
            BMCWEB_LOG_ERROR << "Can't open gz file: " << filename << '\n';
            return false;
        }

        if (!readFile(logStream, skip, top, logEntries, logCount))
        {
            gzclose(logStream);
            return false;
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

    static void printErrorMessage(gzFile logStream)
    {
        int errNum = 0;
        const char* errMsg = gzerror(logStream, &errNum);

        BMCWEB_LOG_ERROR << "Error reading gz compressed data.\n"
                         << "Error Message: " << errMsg << '\n'
                         << "Error Number: " << errNum;
    }

    bool readFile(gzFile logStream, uint64_t skip, uint64_t top,
                  std::vector<std::string>& logEntries, size_t& logCount)
    {
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
            if (!hostLogEntryParser(bufferStr, skip, top, logEntries, logCount))
            {
                BMCWEB_LOG_ERROR << "Error occurs during parsing host log.\n";
                return false;
            }
        } while (gzeof(logStream) != 1);

        return true;
    }

    bool hostLogEntryParser(const std::string& bufferStr, uint64_t skip,
                            uint64_t top, std::vector<std::string>& logEntries,
                            size_t& logCount)
    {
        // Assume we have 8 files, and the max size of each file is
        // 16k, so define the max size as 256kb (double of 8 files *
        // 16kb)
        constexpr size_t maxTotalFilesSize = 262144;

        // It may contain several log entry in one line, and
        // the end of each log entry will be '\r\n' or '\r'.
        // So we need to go through and split string by '\n' and '\r'
        size_t pos = bufferStr.find_first_of("\n\r");
        size_t initialPos = 0;
        std::string newLastMessage;

        while (pos != std::string::npos)
        {
            std::string logEntry =
                bufferStr.substr(initialPos, pos - initialPos);
            // Since there might be consecutive delimiters like "\r\n", we need
            // to filter empty strings.
            if (!logEntry.empty())
            {
                logCount++;
                if (!lastMessage.empty())
                {
                    logEntry.insert(0, lastMessage);
                    lastMessage.clear();
                }
                if (logCount > skip && logCount <= (skip + top))
                {
                    totalFilesSize += logEntry.size();
                    if (totalFilesSize > maxTotalFilesSize)
                    {
                        BMCWEB_LOG_ERROR
                            << "File size exceeds maximum allowed size of "
                            << maxTotalFilesSize;
                        return false;
                    }
                    logEntries.push_back(logEntry);
                }
            }
            else
            {
                // Handle consecutive delimiter. '\r\n' act as a single
                // delimiter, the other case like '\n\n', '\n\r' or '\r\r' will
                // push back a "\n" as a log.
                std::string delimiters;
                if (pos > 0)
                {
                    delimiters = bufferStr.substr(pos - 1, 2);
                }
                // Handle consecutive delimiter but spilt between two files.
                if (pos == 0 && !(lastDelimiter.empty()))
                {
                    delimiters = lastDelimiter + bufferStr.substr(0, 1);
                }
                if (delimiters != "\r\n")
                {
                    logCount++;
                    if (logCount > skip && logCount <= (skip + top))
                    {
                        totalFilesSize++;
                        if (totalFilesSize > maxTotalFilesSize)
                        {
                            BMCWEB_LOG_ERROR
                                << "File size exceeds maximum allowed size of "
                                << maxTotalFilesSize;
                            return false;
                        }
                        logEntries.emplace_back("\n");
                    }
                }
            }
            initialPos = pos + 1;
            pos = bufferStr.find_first_of("\n\r", initialPos);
        }

        // Store the last message
        if (initialPos < bufferStr.size())
        {
            newLastMessage = bufferStr.substr(initialPos);
        }
        // If consecutive delimiter spilt by buffer or file, the last character
        // must be the delimiter.
        else if (initialPos == bufferStr.size())
        {
            lastDelimiter = std::string(1, bufferStr.back());
        }
        // If file doesn't contain any "\r" or "\n", initialPos should be zero
        if (initialPos == 0)
        {
            // Solved an edge case that the log doesn't in skip and top range,
            // but consecutive files don't contain a single delimiter, this
            // lastMessage becomes unnecessarily large. Since last message will
            // prepend to next log, logCount need to plus 1
            if ((logCount + 1) > skip && (logCount + 1) <= (skip + top))
            {
                lastMessage.insert(
                    lastMessage.end(),
                    std::make_move_iterator(newLastMessage.begin()),
                    std::make_move_iterator(newLastMessage.end()));

                // Following the previous question, protect lastMessage don't
                // larger than max total files size
                size_t tmpMessageSize = totalFilesSize + lastMessage.size();
                if (tmpMessageSize > maxTotalFilesSize)
                {
                    BMCWEB_LOG_ERROR
                        << "File size exceeds maximum allowed size of "
                        << maxTotalFilesSize;
                    return false;
                }
            }
        }
        else
        {
            if (!newLastMessage.empty())
            {
                lastMessage = std::move(newLastMessage);
            }
        }
        return true;
    }

  public:
    GzFileReader() = default;
    ~GzFileReader() = default;
    GzFileReader(const GzFileReader&) = delete;
    GzFileReader& operator=(const GzFileReader&) = delete;
    GzFileReader(GzFileReader&&) = delete;
    GzFileReader& operator=(GzFileReader&&) = delete;
};
