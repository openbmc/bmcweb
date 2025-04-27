// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "duplicatable_file_handle.hpp"
#include "logging.hpp"

#include <boost/beast/core/file_posix.hpp>
#include <boost/beast/http/fields.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

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
    ERROR_OUT_OF_RANGE,
    ERROR_DATA_AFTER_TRAILER,
    ERROR_FILE_OPERATION,
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
    END,
    ERROR
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
    std::variant<std::string, DuplicatableFileHandle> content;
    bool isFile = false;
};

class MultipartParser
{
  public:
    MultipartParser() = default;
    MultipartParser(const MultipartParser&) = delete;
    MultipartParser& operator=(const MultipartParser&) = delete;
    MultipartParser(MultipartParser&&) = default;
    MultipartParser& operator=(MultipartParser&&) = default;
    ~MultipartParser() = default;

    [[nodiscard]] ParserError parse(std::string_view contentType,
                                    std::string_view body)
    {
        ParserError ret = start(contentType);
        if (ret != ParserError::PARSER_SUCCESS)
        {
            return ret;
        }

        ret = parsePart(body);
        if (ret != ParserError::PARSER_SUCCESS)
        {
            return ret;
        }
        return finish();
    }

    [[nodiscard]] ParserError start(std::string_view contentType)
    {
        const std::string_view boundaryFormat =
            "multipart/form-data; boundary=";
        if (!contentType.starts_with(boundaryFormat))
        {
            state = State::ERROR;
            return ParserError::ERROR_BOUNDARY_FORMAT;
        }

        std::string_view ctBoundary = contentType.substr(boundaryFormat.size());

        boundary = "\r\n--";
        boundary += ctBoundary;
        indexBoundary();
        lookbehind.resize(boundary.size() + 8);
        state = State::START;
        return ParserError::PARSER_SUCCESS;
    }

    ParserError finish()
    {
        if (state != State::END)
        {
            state = State::ERROR;
            return ParserError::ERROR_UNEXPECTED_END_OF_INPUT;
        }

        return ParserError::PARSER_SUCCESS;
    }

    ParserError parsePart(std::string_view buffer)
    {
        if (state == State::END)
        {
            return ParserError::PARSER_SUCCESS;
        }

        size_t len = buffer.size();
        for (size_t i = 0; i < len; ++i)
        {
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
                            state = State::ERROR;
                            return ParserError::ERROR_BOUNDARY_CR;
                        }
                        index++;
                        break;
                    }
                    else if (index - 1 == boundary.size() - 2)
                    {
                        if (c != lf)
                        {
                            state = State::ERROR;
                            return ParserError::ERROR_BOUNDARY_LF;
                        }
                        index = 0;
                        mime_fields.emplace_back();
                        state = State::HEADER_FIELD_START;
                        break;
                    }
                    if (c != boundary[index + 2])
                    {
                        state = State::ERROR;
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
                {
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
                            state = State::ERROR;
                            return ParserError::ERROR_EMPTY_HEADER;
                        }

                        currentHeaderName.append(&buffer[headerFieldMark],
                                                 i - headerFieldMark);
                        state = State::HEADER_VALUE_START;
                        break;
                    }
                    char cl = lower(c);
                    if (cl < 'a' || cl > 'z')
                    {
                        state = State::ERROR;
                        return ParserError::ERROR_HEADER_NAME;
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
                        std::string_view value(&buffer[headerValueMark],
                                               i - headerValueMark);
                        mime_fields.rbegin()->fields.set(currentHeaderName,
                                                         value);
                        state = State::HEADER_VALUE_ALMOST_DONE;
                    }
                    break;
                }
                case State::HEADER_VALUE_ALMOST_DONE:
                {
                    if (c != lf)
                    {
                        state = State::ERROR;
                        return ParserError::ERROR_HEADER_VALUE;
                    }
                    state = State::HEADER_FIELD_START;
                    break;
                }
                case State::HEADERS_ALMOST_DONE:
                {
                    if (c != lf)
                    {
                        state = State::ERROR;
                        return ParserError::ERROR_HEADER_ENDING;
                    }
                    if (index > 0)
                    {
                        state = State::ERROR;
                        return ParserError::ERROR_UNEXPECTED_END_OF_HEADER;
                    }

                    // Check once if this is a file upload based on
                    // Content-Disposition
                    const auto& fields = mime_fields.back().fields;
                    auto it = fields.find("Content-Disposition");
                    if (it != fields.end() &&
                        it->value().find("filename=") != std::string::npos)
                    {
                        mime_fields.back().isFile = true;
                    }

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
                    if (index == 0)
                    {
                        skipNonBoundary(buffer, boundary.size() - 1, i);
                        if (i >= len)
                        {
                            break;
                        }
                        c = buffer[i];
                    }
                    if (auto ec = processPartData(buffer, i, c);
                        ec != ParserError::PARSER_SUCCESS)
                    {
                        return ec;
                    }
                    break;
                }
                case State::END:
                {
                    // ignore anything after the closing delimiter.
                    i = len;
                    break;
                }
                case State::ERROR:
                {
                    return ParserError::ERROR_DATA_AFTER_TRAILER;
                }
                default:
                {
                    state = State::ERROR;
                    return ParserError::ERROR_UNEXPECTED_END_OF_INPUT;
                }
            }
        }
        /* ------------------------------------------------------------------
         * Flush remainder **except** the still-unverified boundary prefix.
         * 'index' holds the number of chars of <boundary> already matched.
         * These bytes live at the very end of the current buffer and must
         * NOT be committed yet – they will be re-examined with the next
         * network chunk.  Without this guard we replicated those bytes,
         * inflating the output by exactly "<CRLF>--<boundary>--<CRLF>".
         * ------------------------------------------------------------------ */
        if (state == State::PART_DATA)
        {
            size_t flushEnd = len;
            if (index > 0)
            {
                flushEnd = len - index;
            }

            if (partDataMark < flushEnd)
            {
                if (auto ec = writeData(
                        buffer.substr(partDataMark, flushEnd - partDataMark));
                    ec != ParserError::PARSER_SUCCESS)
                {
                    return ec;
                }
            }
            partDataMark = 0;
        }

        return ParserError::PARSER_SUCCESS;
    }
    std::vector<FormPart> mime_fields;
    std::string boundary;

  private:
    void indexBoundary()
    {
        std::ranges::fill(boundaryIndex, 0);
        for (const char current : boundary)
        {
            boundaryIndex[static_cast<unsigned char>(current)] = true;
        }
    }

    static char lower(char c)
    {
        return static_cast<char>(c | 0x20);
    }

    bool isBoundaryChar(char c) const
    {
        return boundaryIndex[static_cast<unsigned char>(c)];
    }

    void skipNonBoundary(std::string_view buffer, size_t boundaryEnd, size_t& i)
    {
        // boyer-moore derived algorithm to safely skip non-boundary data
        while (i + boundary.size() <= buffer.length())
        {
            if (isBoundaryChar(buffer[i + boundaryEnd]))
            {
                break;
            }
            i += boundary.size();
        }
    }

    ParserError writeData(std::string_view buffer)
    {
        auto& content = mime_fields.back().content;

        if (auto* str = std::get_if<std::string>(&content))
        {
            if (mime_fields.back().isFile)
            {
                DuplicatableFileHandle tmp;
                if (!tmp.createTemporaryFile())
                {
                    BMCWEB_LOG_ERROR("Failed to create temporary file");
                    return ParserError::ERROR_FILE_OPERATION;
                }
                boost::system::error_code ec;

                if (!str->empty())
                {
                    tmp.fileHandle.write(str->data(), str->size(), ec);
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR("Failed to write existing data: {}",
                                         ec.message());
                        return ParserError::ERROR_FILE_OPERATION;
                    }
                }

                tmp.fileHandle.write(buffer.data(), buffer.size(), ec);
                if (ec)
                {
                    BMCWEB_LOG_ERROR("Failed to write multipart data: {}",
                                     ec.message());
                    return ParserError::ERROR_FILE_OPERATION;
                }

                str->clear();
                content = std::move(tmp);
            }
            else
            {
                str->append(buffer);
            }
            return ParserError::PARSER_SUCCESS;
        }

        if (auto* file = std::get_if<DuplicatableFileHandle>(&content))
        {
            boost::system::error_code ec;
            file->fileHandle.write(buffer.data(), buffer.size(), ec);
            if (ec)
            {
                BMCWEB_LOG_ERROR("Failed to write multipart data: {}",
                                 ec.message());
                return ParserError::ERROR_FILE_OPERATION;
            }
        }
        return ParserError::PARSER_SUCCESS;
    }

    ParserError processPartData(std::string_view buffer, size_t& i, char c)
    {
        size_t prevIndex = index;

        if (index < boundary.size())
        {
            if (boundary[index] == c)
            {
                if (index == 0)
                {
                    const char* start = &buffer[partDataMark];
                    size_t size = i - partDataMark;
                    if (auto ec = writeData(std::string_view(start, size));
                        ec != ParserError::PARSER_SUCCESS)
                    {
                        return ec;
                    }
                    partDataMark = i;
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
                    mime_fields.emplace_back();
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

            if (auto ec = writeData(lookbehind.substr(0, prevIndex));
                ec != ParserError::PARSER_SUCCESS)
            {
                return ec;
            }
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
