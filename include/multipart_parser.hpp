#pragma once

#include "http_request.hpp"

#include <boost/beast/http/fields.hpp>

#include <string>
#include <string_view>

enum class ParserError
{
    PARSER_SUCCESS,
    ERROR_BOUNDARY_FORMAT,
    ERROR_BOUNDARY_CR,
    ERROR_BOUNDARY_LF,
    ERROR_BOUNDARY_DATA,
    ERROR_EMPTY_HEADER,
    ERROR_HEADER_NAME,
    ERROR_HEADER_VALUE,
    ERROR_HEADER_ENDING,
    ERROR_UNEXPECTED_END_OF_HEADER,
    ERROR_UNEXPECTED_END_OF_INPUT,
    ERROR_OUT_OF_RANGE
};

enum class State
{
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
    END
};

enum class Boundary
{
    NON_BOUNDARY,
    PART_BOUNDARY,
    END_BOUNDARY,
};

struct FormPart
{
    boost::beast::http::fields fields;
    std::string content;
};

class MultipartParser
{
  public:
    MultipartParser() = default;

    [[nodiscard]] ParserError parse(const crow::Request& req)
    {
        std::string_view contentType = req.getHeaderValue("content-type");

        const std::string boundaryFormat = "multipart/form-data; boundary=";
        if (!contentType.starts_with(boundaryFormat))
        {
            return ParserError::ERROR_BOUNDARY_FORMAT;
        }

        std::string_view ctBoundary = contentType.substr(boundaryFormat.size());

        boundary = "\r\n--";
        boundary += ctBoundary;
        indexBoundary();
        lookbehind.resize(boundary.size() + 8);
        state = State::START;

        const char* buffer = req.body().data();
        size_t len = req.body().size();
        char cl = 0;

        for (size_t i = 0; i < len; i++)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            char c = buffer[i];
            switch (state)
            {
                case State::START:
                    index = 0;
                    state = State::START_BOUNDARY;
                    [[fallthrough]];
                case State::START_BOUNDARY:
                    if (index == boundary.size() - 2)
                    {
                        if (c != cr)
                        {
                            return ParserError::ERROR_BOUNDARY_CR;
                        }
                        index++;
                        break;
                    }
                    else if (index - 1 == boundary.size() - 2)
                    {
                        if (c != lf)
                        {
                            return ParserError::ERROR_BOUNDARY_LF;
                        }
                        index = 0;
                        mime_fields.push_back({});
                        state = State::HEADER_FIELD_START;
                        break;
                    }
                    if (c != boundary[index + 2])
                    {
                        return ParserError::ERROR_BOUNDARY_DATA;
                    }
                    index++;
                    break;
                case State::HEADER_FIELD_START:
                    currentHeaderName.resize(0);
                    state = State::HEADER_FIELD;
                    headerFieldMark = i;
                    index = 0;
                    [[fallthrough]];
                case State::HEADER_FIELD:
                    if (c == cr)
                    {
                        headerFieldMark = 0;
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
                            return ParserError::ERROR_EMPTY_HEADER;
                        }

                        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                        currentHeaderName.append(buffer + headerFieldMark,
                                                 i - headerFieldMark);
                        state = State::HEADER_VALUE_START;
                        break;
                    }
                    cl = lower(c);
                    if (cl < 'a' || cl > 'z')
                    {
                        return ParserError::ERROR_HEADER_NAME;
                    }
                    break;
                case State::HEADER_VALUE_START:
                    if (c == space)
                    {
                        break;
                    }
                    headerValueMark = i;
                    state = State::HEADER_VALUE;
                    [[fallthrough]];
                case State::HEADER_VALUE:
                    if (c == cr)
                    {
                        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                        std::string_view value(buffer + headerValueMark,
                                               i - headerValueMark);
                        mime_fields.rbegin()->fields.set(currentHeaderName,
                                                         value);
                        state = State::HEADER_VALUE_ALMOST_DONE;
                    }
                    break;
                case State::HEADER_VALUE_ALMOST_DONE:
                    if (c != lf)
                    {
                        return ParserError::ERROR_HEADER_VALUE;
                    }
                    state = State::HEADER_FIELD_START;
                    break;
                case State::HEADERS_ALMOST_DONE:
                    if (c != lf)
                    {
                        return ParserError::ERROR_HEADER_ENDING;
                    }
                    if (index > 0)
                    {
                        return ParserError::ERROR_UNEXPECTED_END_OF_HEADER;
                    }
                    state = State::PART_DATA_START;
                    break;
                case State::PART_DATA_START:
                    state = State::PART_DATA;
                    partDataMark = i;
                    [[fallthrough]];
                case State::PART_DATA:
                {
                    if (index == 0)
                    {
                        skipNonBoundary(buffer, len, boundary.size() - 1, i);

                        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                        c = buffer[i];
                    }
                    const ParserError ec = processPartData(buffer, i, c);
                    if (ec != ParserError::PARSER_SUCCESS)
                    {
                        return ec;
                    }
                    break;
                }
                case State::END:
                    break;
            }
        }

        if (state != State::END)
        {
            return ParserError::ERROR_UNEXPECTED_END_OF_INPUT;
        }

        return ParserError::PARSER_SUCCESS;
    }
    std::vector<FormPart> mime_fields;
    std::string boundary;

  private:
    void indexBoundary()
    {
        std::fill(boundaryIndex.begin(), boundaryIndex.end(), 0);
        for (const char current : boundary)
        {
            boundaryIndex[static_cast<unsigned char>(current)] = true;
        }
    }

    static char lower(char c)
    {
        return static_cast<char>(c | 0x20);
    }

    inline bool isBoundaryChar(char c) const
    {
        return boundaryIndex[static_cast<unsigned char>(c)];
    }

    void skipNonBoundary(const char* buffer, size_t len, size_t boundaryEnd,
                         size_t& i)
    {
        // boyer-moore derived algorithm to safely skip non-boundary data
        while (i + boundary.size() <= len)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (isBoundaryChar(buffer[i + boundaryEnd]))
            {
                break;
            }
            i += boundary.size();
        }
    }

    ParserError processPartData(const char* buffer, size_t& i, char c)
    {
        size_t prevIndex = index;

        if (index < boundary.size())
        {
            if (boundary[index] == c)
            {
                if (index == 0)
                {
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    const char* start = buffer + partDataMark;
                    size_t size = i - partDataMark;
                    mime_fields.rbegin()->content +=
                        std::string_view(start, size);
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
            if (c == cr)
            {
                // cr = part boundary
                flags = Boundary::PART_BOUNDARY;
            }
            else if (c == hyphen)
            {
                // hyphen = end boundary
                flags = Boundary::END_BOUNDARY;
            }
            else
            {
                index = 0;
            }
        }
        else
        {
            if (flags == Boundary::PART_BOUNDARY)
            {
                index = 0;
                if (c == lf)
                {
                    // unset the PART_BOUNDARY flag
                    flags = Boundary::NON_BOUNDARY;
                    mime_fields.push_back({});
                    state = State::HEADER_FIELD_START;
                    return ParserError::PARSER_SUCCESS;
                }
            }
            if (flags == Boundary::END_BOUNDARY)
            {
                if (c == hyphen)
                {
                    state = State::END;
                }
                else
                {
                    flags = Boundary::NON_BOUNDARY;
                    index = 0;
                }
            }
        }

        if (index > 0)
        {
            if ((index - 1) >= lookbehind.size())
            {
                // Should never happen, but when it does it won't cause crash
                return ParserError::ERROR_OUT_OF_RANGE;
            }
            lookbehind[index - 1] = c;
        }
        else if (prevIndex > 0)
        {
            // if our boundary turned out to be rubbish, the captured
            // lookbehind belongs to partData

            mime_fields.rbegin()->content += lookbehind.substr(0, prevIndex);
            partDataMark = i;

            // reconsider the current character even so it interrupted
            // the sequence it could be the beginning of a new sequence
            i--;
        }
        return ParserError::PARSER_SUCCESS;
    }

    std::string currentHeaderName;
    std::string currentHeaderValue;

    static constexpr char cr = '\r';
    static constexpr char lf = '\n';
    static constexpr char space = ' ';
    static constexpr char hyphen = '-';
    static constexpr char colon = ':';

    std::array<bool, 256> boundaryIndex{};
    std::string lookbehind;
    State state{State::START};
    Boundary flags{Boundary::NON_BOUNDARY};
    size_t index = 0;
    size_t partDataMark = 0;
    size_t headerFieldMark = 0;
    size_t headerValueMark = 0;
};
