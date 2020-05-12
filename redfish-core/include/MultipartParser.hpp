#pragma once

#include <sys/types.h>

#include <cstring>
#include <stdexcept>
#include <string>

enum class State
{
    ERROR,
    START,
    START_BOUNDARY,
    HEADER_FIELD_START,
    HEADER_FIELD,
    HEADER_VALUE_START,
    HEADER_VALUE,
    HEADER_VALUE_ALMOST_DONE,
    HEADERS_ALMOST_DONE,
    PART_DATA_START,
    PART_DATA,
    PART_END,
    END
};

enum class Flags
{
    NONE,
    PART_BOUNDARY,
    LAST_BOUNDARY
};

class MultipartParser
{
  public:
    typedef void (*Callback)(const char* buffer, size_t start, size_t end,
                             void* userData);

  private:
    static const char cr = 13;
    static const char lf = 10;
    static const char space = 32;
    static const char hyphen = 45;
    static const char colon = 58;
    static const size_t unMarked = std::numeric_limits<size_t>::max();

    std::string boundary;
    const char* boundaryData;
    size_t boundarySize;
    std::array<bool, 256> boundaryIndex;
    char* lookbehind;
    size_t lookbehindSize;
    State state;
    Flags flags;
    size_t index;
    size_t headerFieldMark;
    size_t headerValueMark;
    size_t partDataMark;
    const char* errorReason;

    void resetCallbacks()
    {
        onPartBegin = nullptr;
        onHeaderField = nullptr;
        onHeaderValue = nullptr;
        onHeaderEnd = nullptr;
        onHeadersEnd = nullptr;
        onPartData = nullptr;
        onPartEnd = nullptr;
        onEnd = nullptr;
        userData = nullptr;
    }

    void indexBoundary()
    {
        const char* current;
        const char* end = boundaryData + boundarySize;

        std::fill(boundaryIndex.begin(), boundaryIndex.end(), 0);

        for (current = boundaryData; current < end; current++)
        {
            boundaryIndex[static_cast<unsigned char>(*current)] = true;
        }
    }

    void callback(Callback cb, const char* buffer = nullptr,
                  size_t start = unMarked, size_t end = unMarked,
                  bool allowEmpty = false)
    {
        if (start != unMarked && start == end && !allowEmpty)
        {
            return;
        }
        if (cb != nullptr)
        {
            cb(buffer, start, end, userData);
        }
    }

    void dataCallback(Callback cb, size_t& mark, const char* buffer, size_t i,
                      size_t bufferLen, bool clear, bool allowEmpty = false)
    {
        if (mark == unMarked)
        {
            return;
        }

        if (!clear)
        {
            callback(cb, buffer, mark, bufferLen, allowEmpty);
            mark = 0;
        }
        else
        {
            callback(cb, buffer, mark, i, allowEmpty);
            mark = unMarked;
        }
    }

    char lower(char c) const
    {
        return c | 0x20;
    }

    inline bool isBoundaryChar(char c) const
    {
        return boundaryIndex[static_cast<unsigned char>(c)];
    }

    bool isHeaderFieldCharacter(char c) const
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == hyphen;
    }

    void setError(const char* message)
    {
        state = State::ERROR;
        errorReason = message;
    }

    void processPartData(size_t& prevIndex, size_t& index, const char* buffer,
                         size_t len, size_t boundaryEnd, size_t& i, char c,
                         State& state, Flags& flags)
    {
        prevIndex = index;

        if (index == 0)
        {
            // boyer-moore derived algorithm to safely skip non-boundary data
            while (i + boundarySize <= len)
            {
                if (isBoundaryChar(buffer[i + boundaryEnd]))
                {
                    break;
                }

                i += boundarySize;
            }
            if (i == len)
            {
                return;
            }
            c = buffer[i];
        }

        if (index < boundarySize)
        {
            if (boundary[index] == c)
            {
                if (index == 0)
                {
                    dataCallback(onPartData, partDataMark, buffer, i, len,
                                 true);
                }
                index++;
            }
            else
            {
                index = 0;
            }
        }
        else if (index == boundarySize)
        {
            index++;
            if (c == cr)
            {
                // cr = part boundary
                flags = Flags::PART_BOUNDARY;
            }
            else if (c == hyphen)
            {
                // hyphen = end boundary
                flags = Flags::LAST_BOUNDARY;
            }
            else
            {
                index = 0;
            }
        }
        else if (index - 1 == boundarySize)
        {
            if (flags == Flags::PART_BOUNDARY)
            {
                index = 0;
                if (c == lf)
                {
                    // unset the PART_BOUNDARY flag
                    flags = Flags::NONE;
                    callback(onPartEnd);
                    callback(onPartBegin);
                    state = State::HEADER_FIELD_START;
                    return;
                }
            }
            else if (flags == Flags::LAST_BOUNDARY)
            {
                if (c == hyphen)
                {
                    callback(onPartEnd);
                    callback(onEnd);
                    state = State::END;
                }
                else
                {
                    index = 0;
                }
            }
            else
            {
                index = 0;
            }
        }
        else if (index - 2 == boundarySize)
        {
            if (c == cr)
            {
                index++;
            }
            else
            {
                index = 0;
            }
        }
        else if (index - boundarySize == 3)
        {
            index = 0;
            if (c == lf)
            {
                callback(onPartEnd);
                callback(onEnd);
                state = State::END;
                return;
            }
        }

        if (index > 0)
        {
            // when matching a possible boundary, keep a lookbehind reference
            // in case it turns out to be a false lead
            if (index - 1 >= lookbehindSize)
            {
                setError("Parser bug: index overflows lookbehind buffer. "
                         "Please send bug report with input file attached.");
                throw std::out_of_range("index overflows lookbehind buffer");
            }
            else if (index == 0)
            {
                setError("Parser bug: index underflows lookbehind buffer. "
                         "Please send bug report with input file attached.");
                throw std::out_of_range("index underflows lookbehind buffer");
            }
            lookbehind[index - 1] = c;
        }
        else if (prevIndex > 0)
        {
            // if our boundary turned out to be rubbish, the captured lookbehind
            // belongs to partData
            callback(onPartData, lookbehind, 0, prevIndex);
            prevIndex = 0;
            partDataMark = i;

            // reconsider the current character even so it interrupted the
            // sequence it could be the beginning of a new sequence
            i--;
        }
    }

  public:
    Callback onPartBegin;
    Callback onHeaderField;
    Callback onHeaderValue;
    Callback onHeaderEnd;
    Callback onHeadersEnd;
    Callback onPartData;
    Callback onPartEnd;
    Callback onEnd;
    void* userData;

    MultipartParser()
    {
        lookbehind = NULL;
        resetCallbacks();
        reset();
    }

    MultipartParser(const std::string& boundary)
    {
        lookbehind = NULL;
        resetCallbacks();
        setBoundary(boundary);
    }

    ~MultipartParser()
    {
        delete[] lookbehind;
    }

    void reset()
    {
        delete[] lookbehind;
        state = State::ERROR;
        boundary.clear();
        boundaryData = boundary.c_str();
        boundarySize = 0;
        lookbehind = NULL;
        lookbehindSize = 0;
        flags = Flags::NONE;
        index = 0;
        headerFieldMark = unMarked;
        headerValueMark = unMarked;
        partDataMark = unMarked;
        errorReason = "Parser uninitialized.";
    }

    void setBoundary(const std::string& boundary)
    {
        reset();
        this->boundary = "\r\n--" + boundary;
        boundaryData = this->boundary.c_str();
        boundarySize = this->boundary.size();
        indexBoundary();
        lookbehind = new char[boundarySize + 8];
        lookbehindSize = boundarySize + 8;
        state = State::START;
        errorReason = "No error.";
    }

    size_t feed(const char* buffer, size_t len)
    {
        if (state == State::ERROR || len == 0)
        {
            return 0;
        }

        State state = this->state;
        Flags flags = this->flags;
        size_t prevIndex = this->index;
        size_t index = this->index;
        size_t boundaryEnd = boundarySize - 1;
        size_t i;
        char c, cl;

        for (i = 0; i < len; i++)
        {
            c = buffer[i];

            switch (state)
            {
                case State::ERROR:
                    return i;
                case State::START:
                {
                    index = 0;
                    state = State::START_BOUNDARY;
                    [[fallthrough]];
                }
                case State::START_BOUNDARY:
                {
                    if (index == boundarySize - 2)
                    {
                        if (c != cr)
                        {
                            setError("Malformed. Expected cr after boundary.");
                            return i;
                        }
                        index++;
                        break;
                    }
                    else if (index - 1 == boundarySize - 2)
                    {
                        if (c != lf)
                        {
                            setError(
                                "Malformed. Expected lf after boundary cr.");
                            return i;
                        }
                        index = 0;
                        callback(onPartBegin);
                        state = State::HEADER_FIELD_START;
                        break;
                    }
                    if (c != boundary[index + 2])
                    {
                        setError("Malformed. Found different boundary data "
                                 "than the given one.");
                        return i;
                    }
                    index++;
                    break;
                }
                case State::HEADER_FIELD_START:
                {
                    state = State::HEADER_FIELD;
                    headerFieldMark = i;
                    index = 0;
                    [[fallthrough]];
                }
                case State::HEADER_FIELD:
                {
                    if (c == cr)
                    {
                        headerFieldMark = unMarked;
                        state = State::HEADERS_ALMOST_DONE;
                        break;
                    }

                    index++;
                    if (c == hyphen)
                    {
                        break;
                    }

                    if (c == colon)
                    {
                        if (index == 1)
                        {
                            // empty header field
                            setError("Malformed first header name character.");
                            return i;
                        }
                        dataCallback(onHeaderField, headerFieldMark, buffer, i,
                                     len, true);
                        state = State::HEADER_VALUE_START;
                        break;
                    }

                    cl = lower(c);
                    if (cl < 'a' || cl > 'z')
                    {
                        setError("Malformed header name.");
                        return i;
                    }
                    break;
                }
                case State::HEADER_VALUE_START:
                {
                    if (c == space)
                    {
                        break;
                    }

                    headerValueMark = i;
                    state = State::HEADER_VALUE;
                    [[fallthrough]];
                }
                case State::HEADER_VALUE:
                {
                    if (c == cr)
                    {
                        dataCallback(onHeaderValue, headerValueMark, buffer, i,
                                     len, true, true);
                        callback(onHeaderEnd);
                        state = State::HEADER_VALUE_ALMOST_DONE;
                    }
                    break;
                }
                case State::HEADER_VALUE_ALMOST_DONE:
                {
                    if (c != lf)
                    {
                        setError(
                            "Malformed header value: lf expected after cr");
                        return i;
                    }

                    state = State::HEADER_FIELD_START;
                    break;
                }
                case State::HEADERS_ALMOST_DONE:
                {
                    if (c != lf)
                    {
                        setError(
                            "Malformed header ending: lf expected after cr");
                        return i;
                    }

                    callback(onHeadersEnd);
                    state = State::PART_DATA_START;
                    break;
                }
                case State::PART_DATA_START:
                {
                    state = State::PART_DATA;
                    partDataMark = i;
                    [[fallthrough]];
                }
                case State::PART_DATA:
                {
                    processPartData(prevIndex, index, buffer, len, boundaryEnd,
                                    i, c, state, flags);
                    break;
                }
                default:
                    return i;
            }
        }

        dataCallback(onHeaderField, headerFieldMark, buffer, i, len, false);
        dataCallback(onHeaderValue, headerValueMark, buffer, i, len, false);
        dataCallback(onPartData, partDataMark, buffer, i, len, false);

        this->index = index;
        this->state = state;
        this->flags = flags;

        return len;
    }

    bool succeeded() const
    {
        return state == State::END;
    }

    bool hasError() const
    {
        return state == State::ERROR;
    }

    bool stopped() const
    {
        return state == State::ERROR || state == State::END;
    }

    const char* getErrorMessage() const
    {
        return errorReason;
    }
};
