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
        gzFile logStream;
        std::string wholeFile;
        std::string newLastMessage;

        logStream = gzopen(filename.c_str(), "r");
        if (!logStream)
        {
            BMCWEB_LOG_ERROR << "Can't open gz file: " << filename << '\n';
            return false;
        }

        if (!readFile(wholeFile, logStream))
        {
            gzclose(logStream);
            return false;
        }

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
            logEntries.insert(logEntries.end(), parseLogs.begin(),
                              parseLogs.end());
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
            wholeFile += bufferStr;
        } while (!gzeof(logStream));

        return true;
    }

    std::vector<std::string> hostLogEntryParser(const std::string& wholeFile,
                                                std::string& newLastMessage)
    {
        std::string logEntry;
        std::vector<std::string> logEntries;

        // It may contain several log entry in one line, and
        // the end of each log entry will be '\r\n' or '\r'.
        // So we need to go through and split string by '\n' and '\r'
        size_t pos = wholeFile.find_first_of("\n\r");
        size_t initialPos = 0;

        while (pos != std::string::npos)
        {
            logEntry = wholeFile.substr(initialPos, pos - initialPos);
            // Since there might be consecutive delimiters like "\r\n", we need
            // to filter empty strings.
            if (!logEntry.empty())
            {
                logEntries.push_back(logEntry);
            }
            initialPos = pos + 1;
            pos = wholeFile.find_first_of("\n\r", initialPos);
        }

        // Store the last message
        if (initialPos < wholeFile.size())
        {
            newLastMessage = wholeFile.substr(initialPos);
        }

        return logEntries;
    }

  public:
    GzFileReader() = default;
    ~GzFileReader() = default;
    GzFileReader(const GzFileReader&) = delete;
    GzFileReader& operator=(const GzFileReader&) = delete;
};
