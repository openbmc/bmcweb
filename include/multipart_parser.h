#pragma once

#include <http_request.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/beast/http/fields.hpp>

#include <string>
#include <string_view>

class MultipartParser
{
  private:
    std::string currentHeaderName;
    std::string currentHeaderValue;

    crow::Request& req;

    static const char CR = 13;
    static const char LF = 10;
    static const char SPACE = 32;
    static const char HYPHEN = 45;
    static const char COLON = 58;
    static const size_t UNMARKED = -1U;

    enum State
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

    enum Flags
    {
        PART_BOUNDARY = 1,
        LAST_BOUNDARY = 2
    };

    std::string boundary;
    std::array<bool, 256> boundaryIndex;
    std::string lookbehind;
    State state;
    int flags;
    size_t index;
    size_t headerFieldMark;
    size_t headerValueMark;
    size_t partDataMark;
    std::string_view errorReason;

    void indexBoundary()
    {
        std::fill(boundaryIndex.begin(), boundaryIndex.end(), 0);
        for (const char current : boundary)
        {
            boundaryIndex[static_cast<size_t>(current)] = true;
        }
    }

    char lower(char c) const
    {
        return c | 0x20;
    }

    inline bool isBoundaryChar(char c) const
    {
        return boundaryIndex[static_cast<size_t>(c)];
    }

    void setError(const std::string_view message)
    {
        state = ERROR;
        errorReason = message;
    }

    void processPartData(size_t& prevIndex, size_t& index, const char* buffer,
                         size_t len, size_t boundaryEnd, size_t& i, char c,
                         State& state, int& flags)
    {
        prevIndex = index;

        if (index == 0)
        {
            // boyer-moore derived algorithm to safely skip non-boundary data
            while (i + boundary.size() <= len)
            {
                if (isBoundaryChar(buffer[i + boundaryEnd]))
                {
                    break;
                }

                i += boundary.size();
            }
            if (i == len)
            {
                return;
            }
            c = buffer[i];
        }

        if (index < boundary.size())
        {
            if (boundary[index] == c)
            {
                if (index == 0)
                {
                    if (partDataMark != UNMARKED)
                    {
                        req.mime_fields.rbegin()->content += std::string_view(
                            buffer + partDataMark, i - partDataMark);
                    }
                }
                index++;
            }
            else
            {
                index = 0;
            }
        }
        else if (index == boundary.size())
        {
            index++;
            if (c == CR)
            {
                // CR = part boundary
                flags |= PART_BOUNDARY;
            }
            else if (c == HYPHEN)
            {
                // HYPHEN = end boundary
                flags |= LAST_BOUNDARY;
            }
            else
            {
                index = 0;
            }
        }
        else if (index - 1 == boundary.size())
        {
            if (flags & PART_BOUNDARY)
            {
                index = 0;
                if (c == LF)
                {
                    // unset the PART_BOUNDARY flag
                    flags &= ~PART_BOUNDARY;
                    req.mime_fields.push_back({});
                    state = HEADER_FIELD_START;
                    return;
                }
            }
            else if (flags & LAST_BOUNDARY)
            {
                if (c == HYPHEN)
                {
                    // callback(onPartEnd);
                    // callback(onEnd);
                    state = END;
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
        else if (index - 2 == boundary.size())
        {
            if (c == CR)
            {
                index++;
            }
            else
            {
                index = 0;
            }
        }
        else if (index - boundary.size() == 3)
        {
            index = 0;
            if (c == LF)
            {
                // callback(onPartEnd);
                // callback(onEnd);
                state = END;
                return;
            }
        }

        if (index > 0)
        {
            // when matching a possible boundary, keep a lookbehind reference
            // in case it turns out to be a false lead
            if (index - 1U >= lookbehind.size())
            {
                setError("Parser bug: index overflows lookbehind buffer. "
                         "Please send bug report with input file attached.");
                return;
            }
            else if (index == 0)
            {
                setError("Parser bug: index underflows lookbehind buffer. "
                         "Please send bug report with input file attached.");
                return;
            }
            lookbehind[index - 1] = c;
        }
        else if (prevIndex > 0)
        {
            // if our boundary turned out to be rubbish, the captured lookbehind
            // belongs to partData

            req.mime_fields.rbegin()->content += lookbehind.substr(prevIndex);
            prevIndex = 0;
            partDataMark = i;

            // reconsider the current character even so it interrupted the
            // sequence it could be the beginning of a new sequence
            i--;
        }
    }

  public:
    MultipartParser(crow::Request& reqIn) : req(reqIn)
    {
        reset();
        std::string_view contentType = req.getHeaderValue("content-type");

        if (!boost::starts_with(contentType, "multipart/form-data; boundary="))
        {
            return;
        }

        std::string_view ct_boundary =
            contentType.substr(sizeof("multipart/form-data; boundary=") - 1);

        BMCWEB_LOG_DEBUG << "Boundary is \"" << ct_boundary << "\"";
        boundary = "\r\n--";
        boundary += ct_boundary;
        indexBoundary();
        lookbehind.resize(boundary.size() + 8);
        state = START;
    }

    ~MultipartParser()
    {}

    void reset()
    {
        state = ERROR;
        boundary.clear();
        lookbehind.resize(0);
        flags = 0;
        index = 0;
        currentHeaderName.resize(0);
        currentHeaderValue.resize(0);
        headerFieldMark = UNMARKED;
        headerValueMark = UNMARKED;
        partDataMark = UNMARKED;
        errorReason = "Parser uninitialized.";
    }

    size_t parse()
    {
        const char* buffer = req.body.data();
        size_t len = req.body.size();

        if (state == ERROR || len == 0)
        {
            return 0;
        }

        size_t prevIndex = this->index;
        char cl = 0;
        size_t i = 0;

        for (i = 0; i < len; i++)
        {
            char c = buffer[i];

            BMCWEB_LOG_DEBUG << "Parse state " << static_cast<int>(state)
                             << "  current char=" << c << " index = " << i;
            switch (state)
            {
                case ERROR:
                    return i;
                case START:
                    index = 0;
                    state = START_BOUNDARY;
                    [[fallthrough]];
                case START_BOUNDARY:
                    if (index == boundary.size() - 2)
                    {
                        if (c != CR)
                        {
                            setError("Malformed. Expected CR after boundary.");
                            return i;
                        }
                        index++;
                        break;
                    }
                    else if (index - 1 == boundary.size() - 2)
                    {
                        if (c != LF)
                        {
                            setError(
                                "Malformed. Expected LF after boundary CR.");
                            return i;
                        }
                        index = 0;

                        req.mime_fields.push_back({});

                        state = HEADER_FIELD_START;
                        break;
                    }
                    if (c != boundary[index + 2])
                    {
                        setError("Malformed. Found different boundary data "
                                 "than the given one.");
                        BMCWEB_LOG_DEBUG << "Got " << c << "expecting "
                                         << boundary[index + 2];
                        return i;
                    }
                    index++;
                    break;
                case HEADER_FIELD_START:
                    currentHeaderName.resize(0);
                    state = HEADER_FIELD;
                    headerFieldMark = i;
                    index = 0;
                    [[fallthrough]];
                case HEADER_FIELD:
                    if (c == CR)
                    {
                        headerFieldMark = UNMARKED;
                        state = HEADERS_ALMOST_DONE;
                        break;
                    }

                    index++;
                    if (c == HYPHEN)
                    {
                        break;
                    }

                    if (c == COLON)
                    {
                        if (index == 1)
                        {
                            // empty header field
                            setError("Malformed first header name character.");
                            return i;
                        }
                        currentHeaderName.append(buffer + headerFieldMark,
                                                 i - headerFieldMark);
                        state = HEADER_VALUE_START;
                        break;
                    }

                    cl = lower(c);
                    if (cl < 'a' || cl > 'z')
                    {
                        setError("Malformed header name.");
                        return i;
                    }
                    break;
                case HEADER_VALUE_START:
                    if (c == SPACE)
                    {
                        break;
                    }

                    headerValueMark = i;
                    state = HEADER_VALUE;
                    [[fallthrough]];
                case HEADER_VALUE:
                    if (c == CR)
                    {
                        std::string_view value(buffer + headerValueMark,
                                               i - headerValueMark);
                        BMCWEB_LOG_DEBUG << "Found header " << currentHeaderName
                                         << "=" << value;
                        req.mime_fields.rbegin()->fields.set(currentHeaderName,
                                                             value);

                        state = HEADER_VALUE_ALMOST_DONE;
                    }
                    break;
                case HEADER_VALUE_ALMOST_DONE:
                    if (c != LF)
                    {
                        setError(
                            "Malformed header value: LF expected after CR");
                        return i;
                    }

                    state = HEADER_FIELD_START;
                    break;
                case HEADERS_ALMOST_DONE:
                    if (c != LF)
                    {
                        setError(
                            "Malformed header ending: LF expected after CR");
                        return i;
                    }

                    state = PART_DATA_START;
                    break;
                case PART_DATA_START:
                    state = PART_DATA;
                    partDataMark = i;
                    [[fallthrough]];
                case PART_DATA:

                    processPartData(prevIndex, index, buffer, len,
                                    boundary.size() - 1, i, c, state, flags);
                    break;
                default:
                    return i;
            }
        }

        if (len != 0)
        {
            req.mime_fields.rbegin()->content +=
                std::string_view(buffer + partDataMark, len);
        }

        return len;
    }

    bool succeeded() const
    {
        return state == END;
    }

    bool hasError() const
    {
        return state == ERROR;
    }

    bool stopped() const
    {
        return state == ERROR || state == END;
    }

    const std::string_view getErrorMessage() const
    {
        return errorReason;
    }
};
