// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "logging.hpp"

#include <boost/beast/http/fields.hpp>
#include <boost/container/devector.hpp>
#include <boost/container/options.hpp>

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
    ERROR_EMPTY_HEADER,
    ERROR_HEADER_NAME,
    ERROR_HEADER_NAME_TOO_LONG,
    ERROR_HEADER_VALUE,
    ERROR_HEADER_VALUE_TOO_LONG,
    ERROR_HEADER_ENDING,
    ERROR_UNEXPECTED_END_OF_HEADER,
    ERROR_UNEXPECTED_CHARACTER,
    ERROR_UNEXPECTED_END_OF_INPUT,
    ERROR_OUT_OF_RANGE,
    ERROR_DATA_AFTER_FINAL_BOUNDARY,
    ERROR_DATA_AFTER_ERROR
};

enum class State
{
    START,
    START_BOUNDARY,
    BOUNDARY,
    FIRST_BOUNDARY_CHAR,
    SECOND_BOUNDARY_CHAR_LF,
    SECOND_BOUNDARY_CHAR_DASH,
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

using boost::container::growth_factor;
using boost::container::growth_factor_50;
using boost::container::relocate_on_90;
using boost::container::stored_size;
using bmcweb_buffer_options =
    boost::container::devector_options<growth_factor<growth_factor_50>,
                                       relocate_on_90>::type;

struct FormPart
{
    boost::beast::http::fields fields;

    // Devector is used here because we need to be able to efficiently fill the
    // content, then efficiently "drain" the content in chunks to avoid running
    // the system out of ram.
    using buffer_type =
        boost::container::devector<char, void, bmcweb_buffer_options>;
    buffer_type content;
};

class MultipartParser
{
  public:
    [[nodiscard]] ParserError start(std::string_view contentType,
                                    size_t expectedBytesIn)
    {
        const std::string_view boundaryFormat =
            "multipart/form-data; boundary=";
        if (!contentType.starts_with(boundaryFormat))
        {
            state = State::ERROR;
            return ParserError::ERROR_BOUNDARY_FORMAT;
        }
        std::string_view boundaryStr =
            contentType.substr(boundaryFormat.size());
        boundary = std::format("\r\n--{}", boundaryStr);
        boundary_first = std::format("--{}\r\n", boundaryStr);

        expectedBytes = expectedBytesIn;
        state = State::START;
        return ParserError::PARSER_SUCCESS;
    }

    [[nodiscard]] ParserError parse(std::string_view contentType,
                                    std::string_view body)
    {
        ParserError ret = start(contentType, body.size());
        if (ret != ParserError::PARSER_SUCCESS)
        {
            return ret;
        }
        std::string_view bodyView = body;
        ret = parsePart(bodyView);
        if (ret != ParserError::PARSER_SUCCESS)
        {
            return ret;
        }
        return finish();
    }

    ParserError parsePart(std::span<const char> buffer)
    {
        for (const char c : buffer)
        {
            bytesProcessed++;
            switch (state)
            {
                case State::START:
                    index = 0;
                    state = State::START_BOUNDARY;
                    [[fallthrough]];
                case State::START_BOUNDARY:
                {
                    if (index < boundary_first.size())
                    {
                        if (c != boundary_first[index])
                        {
                            state = State::ERROR;
                            return ParserError::ERROR_BOUNDARY_FORMAT;
                        }
                    }
                    index++;
                    if (index == boundary_first.size())
                    {
                        mime_fields.emplace_back();
                        state = State::HEADER_FIELD_START;
                        index = 0;
                    }

                    break;
                }
                case State::HEADER_FIELD_START:
                    state = State::HEADER_FIELD;
                    index = 0;
                    [[fallthrough]];
                case State::HEADER_FIELD:
                {
                    if (currentHeaderName.size() > 400)
                    {
                        state = State::ERROR;
                        return ParserError::ERROR_HEADER_NAME_TOO_LONG;
                    }
                    if (c == '\r')
                    {
                        state = State::HEADERS_ALMOST_DONE;
                        break;
                    }

                    index++;

                    if (c == ':')
                    {
                        if (currentHeaderName.empty())
                        {
                            state = State::ERROR;
                            return ParserError::ERROR_EMPTY_HEADER;
                        }

                        state = State::HEADER_VALUE_START;
                        break;
                    }
                    char cl = lower(c);
                    if ((cl < 'a' || cl > 'z') && cl != '-')
                    {
                        state = State::ERROR;
                        return ParserError::ERROR_HEADER_NAME;
                    }
                    currentHeaderName.push_back(cl);
                    break;
                }
                case State::HEADER_VALUE_START:
                    if (c == ' ')
                    {
                        break;
                    }
                    state = State::HEADER_VALUE;
                    [[fallthrough]];

                case State::HEADER_VALUE:
                {
                    if (currentHeaderName.size() > 400)
                    {
                        state = State::ERROR;
                        return ParserError::ERROR_HEADER_VALUE_TOO_LONG;
                    }
                    if (c == '\r')
                    {
                        mime_fields.back().fields.set(currentHeaderName,
                                                      currentHeaderValue);
                        currentHeaderValue.clear();
                        currentHeaderName.clear();
                        state = State::HEADER_VALUE_ALMOST_DONE;
                        break;
                    }
                    currentHeaderValue.push_back(c);
                    break;
                }
                case State::HEADER_VALUE_ALMOST_DONE:
                {
                    if (c != '\n')
                    {
                        state = State::ERROR;
                        return ParserError::ERROR_HEADER_VALUE;
                    }
                    state = State::HEADER_FIELD_START;
                    break;
                }
                case State::HEADERS_ALMOST_DONE:
                {
                    if (c != '\n')
                    {
                        state = State::ERROR;
                        return ParserError::ERROR_HEADER_ENDING;
                    }
                    if (index > 0)
                    {
                        state = State::ERROR;
                        return ParserError::ERROR_UNEXPECTED_END_OF_HEADER;
                    }
                    // Assume that the next field contains all the remaining
                    // data. If it doesn't, it will be shrunk again once the
                    // boundary is found.
                    if (expectedBytes > bytesProcessed)
                    {
                        mime_fields.back().content.reserve(
                            expectedBytes - bytesProcessed);
                    }

                    state = State::PART_DATA_START;
                    break;
                }
                case State::PART_DATA_START:
                    state = State::PART_DATA;
                    index = 0;
                    [[fallthrough]];

                case State::PART_DATA:
                {
                    FormPart::buffer_type& content = mime_fields.back().content;
                    content.push_back(c);
                    if (std::string_view(content.data(), content.size())
                            .ends_with(boundary))
                    {
                        state = State::FIRST_BOUNDARY_CHAR;
                    }
                    break;
                }
                case State::FIRST_BOUNDARY_CHAR:
                {
                    FormPart::buffer_type& content = mime_fields.back().content;
                    content.push_back(c);
                    if (c == '\r')
                    {
                        state = State::SECOND_BOUNDARY_CHAR_LF;
                        break;
                    }
                    if (c == '-')
                    {
                        state = State::SECOND_BOUNDARY_CHAR_DASH;
                        break;
                    }
                    state = State::PART_DATA;
                    break;
                }
                case State::SECOND_BOUNDARY_CHAR_LF:
                {
                    FormPart::buffer_type& content = mime_fields.back().content;
                    if (c != '\n')
                    {
                        content.push_back(c);
                        state = State::PART_DATA;
                        break;
                    }
                    content.resize(content.size() - boundary.size() - 1);
                    state = State::HEADER_FIELD_START;
                    index = 0;
                    mime_fields.back().content.shrink_to_fit();
                    mime_fields.emplace_back();
                    break;
                }
                case State::SECOND_BOUNDARY_CHAR_DASH:
                {
                    FormPart::buffer_type& content = mime_fields.back().content;
                    if (c != '-')
                    {
                        content.push_back(c);
                        state = State::PART_DATA;
                        break;
                    }
                    content.resize(content.size() - boundary.size() - 1);
                    state = State::END;
                    index = 0;
                    break;
                }
                case State::END:
                {
                    switch (index)
                    {
                        case 0:
                            if (c != '\r')
                            {
                                return ParserError::
                                    ERROR_DATA_AFTER_FINAL_BOUNDARY;
                            }
                            index++;
                            break;
                        case 1:
                            if (c != '\n')
                            {
                                return ParserError::
                                    ERROR_DATA_AFTER_FINAL_BOUNDARY;
                            }
                            index++;
                            break;
                        default:
                            return ParserError::ERROR_DATA_AFTER_FINAL_BOUNDARY;
                    }
                    break;
                }
                case State::ERROR:
                {
                    return ParserError::ERROR_DATA_AFTER_ERROR;
                }

                default:
                {
                    state = State::ERROR;
                    return ParserError::ERROR_UNEXPECTED_END_OF_INPUT;
                }
            }
        }

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

    std::vector<FormPart> mime_fields;
    std::string boundary;
    std::string boundary_first;

  private:
    static char lower(char c)
    {
        return static_cast<char>(c | 0x20);
    }

    std::string currentHeaderName;
    std::string currentHeaderValue;
    size_t expectedBytes = 0;

    State state = State::START;
    size_t index = 0;
    size_t bytesProcessed = 0;
};
