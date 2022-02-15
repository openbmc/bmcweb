#pragma once

#include <boost/algorithm/string/predicate.hpp>
#include <boost/beast/http/fields.hpp>
#include <http_request.hpp>

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
    ERROR_HEADER_ENDING
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
        if (!boost::starts_with(req.getHeaderValue("content-type"),
                                boundaryFormat))
        {
            return ParserError::ERROR_BOUNDARY_FORMAT;
        }

        std::string_view ctBoundary = contentType.substr(boundaryFormat.size());

        boundary = "\r\n--";
        boundary += ctBoundary;
        indexBoundary();
        lookbehind.resize(boundary.size() + 8);
        state = State::START;

        const char* buffer = req.body.data();
        size_t len = req.body.size();
        size_t prevIndex = index;
        char lowerChar = 0;

        for (size_t i = 0; i < len; i++)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            char currentChar = buffer[i];
            switch (state)
            {
                case State::START:
                    index = 0;
                    state = State::START_BOUNDARY;
                    [[fallthrough]];
                case State::START_BOUNDARY:
                    if (index == boundary.size() - 2)
                    {
                        if (currentChar != '\r')
                        {
                            return ParserError::ERROR_BOUNDARY_CR;
                        }
                        index++;
                        break;
                    }
                    else if (index - 1 == boundary.size() - 2)
                    {
                        if (currentChar != '\n')
                        {
                            return ParserError::ERROR_BOUNDARY_LF;
                        }
                        index = 0;
                        mime_fields.push_back({});
                        state = State::HEADER_FIELD_START;
                        break;
                    }
                    if (currentChar != boundary[index + 2])
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
                    if (currentChar == '\r')
                    {
                        headerFieldMark = 0;
                        state = State::HEADERS_ALMOST_DONE;
                        break;
                    }

                    index++;
                    if (currentChar == '-')
                    {
                        break;
                    }

                    if (currentChar == ':')
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
                    lowerChar = lower(currentChar);
                    if (lowerChar < 'a' || lowerChar > 'z')
                    {
                        return ParserError::ERROR_HEADER_NAME;
                    }
                    break;
                case State::HEADER_VALUE_START:
                    if (currentChar == ' ')
                    {
                        break;
                    }
                    headerValueMark = i;
                    state = State::HEADER_VALUE;
                    [[fallthrough]];
                case State::HEADER_VALUE:
                    if (currentChar == '\r')
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
                    if (currentChar != '\n')
                    {
                        return ParserError::ERROR_HEADER_VALUE;
                    }
                    state = State::HEADER_FIELD_START;
                    break;
                case State::HEADERS_ALMOST_DONE:
                    if (currentChar != '\n')
                    {
                        return ParserError::ERROR_HEADER_ENDING;
                    }
                    state = State::PART_DATA_START;
                    break;
                case State::PART_DATA_START:
                    state = State::PART_DATA;
                    partDataMark = i;
                    [[fallthrough]];
                case State::PART_DATA:
                    if (index == 0)
                    {
                        skipNonBoundary(buffer, len, boundary.size() - 1, i);

                        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                        currentChar = buffer[i];
                    }
                    processPartData(prevIndex, index, buffer, i, currentChar,
                                    state);
                    break;
                case State::END:
                    break;
            }
        }
        return ParserError::PARSER_SUCCESS;
    }
    std::vector<FormPart> mime_fields{};
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

    static char lower(char character)
    {
        return static_cast<char>(character | 0x20);
    }

    inline bool isBoundaryChar(char character) const
    {
        return boundaryIndex[static_cast<unsigned char>(character)];
    }

    void skipNonBoundary(const char* buffer, size_t len, size_t boundaryEnd,
                         size_t& bufferIndex)
    {
        // boyer-moore derived algorithm to safely skip non-boundary data
        while (bufferIndex + boundary.size() <= len)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (isBoundaryChar(buffer[bufferIndex + boundaryEnd]))
            {
                break;
            }
            bufferIndex += boundary.size();
        }
    }

    void processPartData(size_t& prevIndex, size_t& index, const char* buffer,
                         size_t& bufferIndex, char currentChar, State& state)
    {
        prevIndex = index;

        if (index < boundary.size())
        {
            if (boundary[index] == currentChar)
            {
                if (index == 0)
                {
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    const char* start = buffer + partDataMark;
                    size_t size = bufferIndex - partDataMark;
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
            if (currentChar == '\r')
            {
                // cr = part boundary
                flags = Boundary::PART_BOUNDARY;
            }
            else if (currentChar == '-')
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
                if (currentChar == '\n')
                {
                    // unset the PART_BOUNDARY flag
                    flags = Boundary::NON_BOUNDARY;
                    mime_fields.push_back({});
                    state = State::HEADER_FIELD_START;
                    return;
                }
            }
            if (flags == Boundary::END_BOUNDARY)
            {
                if (currentChar == '-')
                {
                    state = State::END;
                }
            }
        }

        if (index > 0)
        {
            lookbehind[index - 1] = currentChar;
        }
        else if (prevIndex > 0)
        {
            // if our boundary turned out to be rubbish, the captured
            // lookbehind belongs to partData

            mime_fields.rbegin()->content += lookbehind.substr(0, prevIndex);
            prevIndex = 0;
            partDataMark = bufferIndex;

            // reconsider the current character even so it interrupted
            // the sequence it could be the beginning of a new sequence
            bufferIndex--;
        }
    }

    std::string currentHeaderName;
    std::string currentHeaderValue;

    std::array<bool, 256> boundaryIndex{};
    std::string lookbehind;
    State state{State::START};
    Boundary flags{Boundary::NON_BOUNDARY};
    size_t index = 0;
    size_t partDataMark = 0;
    size_t headerFieldMark = 0;
    size_t headerValueMark = 0;
};
