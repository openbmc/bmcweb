#include <nlohmann/json.hpp>

namespace json_html_util
{

static constexpr uint8_t UTF8_ACCEPT = 0;
static constexpr uint8_t UTF8_REJECT = 1;

static uint8_t decode(uint8_t& state, uint32_t& codep,
                      const uint8_t byte) noexcept
{
    // clang-format off
    static const std::array<std::uint8_t, 400> utf8d =
    {
        {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 00..1F
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 20..3F
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 40..5F
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 60..7F
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, // 80..9F
            7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, // A0..BF
            8, 8, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // C0..DF
            0xA, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x4, 0x3, 0x3, // E0..EF
            0xB, 0x6, 0x6, 0x6, 0x5, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, // F0..FF
            0x0, 0x1, 0x2, 0x3, 0x5, 0x8, 0x7, 0x1, 0x1, 0x1, 0x4, 0x6, 0x1, 0x1, 0x1, 0x1, // s0..s0
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, // s1..s2
            1, 2, 1, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, // s3..s4
            1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 3, 1, 1, 1, 1, 1, 1, // s5..s6
            1, 3, 1, 1, 1, 1, 1, 3, 1, 3, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 // s7..s8
        }
    };
    // clang-format on

    const uint8_t type = utf8d[byte];

    codep = (state != UTF8_ACCEPT)
                ? (byte & 0x3fu) | (codep << 6)
                : static_cast<uint32_t>(0xff >> type) & (byte);

    state = utf8d[256u + state * 16u + type];
    return state;
}

void dump_escaped(std::string& o, const std::string& s)
{
    std::array<char, 512> string_buffer{{}};
    uint32_t codepoint;
    uint8_t state = UTF8_ACCEPT;
    std::size_t bytes = 0; // number of bytes written to string_buffer

    // number of bytes written at the point of the last valid byte
    std::size_t bytes_after_last_accept = 0;
    std::size_t undumped_chars = 0;

    for (std::size_t i = 0; i < s.size(); ++i)
    {
        const auto byte = static_cast<uint8_t>(s[i]);

        switch (decode(state, codepoint, byte))
        {
            case UTF8_ACCEPT: // decode found a new code point
            {
                switch (codepoint)
                {
                    case 0x08: // backspace
                    {
                        string_buffer[bytes++] = '\\';
                        string_buffer[bytes++] = 'b';
                        break;
                    }

                    case 0x09: // horizontal tab
                    {
                        string_buffer[bytes++] = '\\';
                        string_buffer[bytes++] = 't';
                        break;
                    }

                    case 0x0A: // newline
                    {
                        string_buffer[bytes++] = '\\';
                        string_buffer[bytes++] = 'n';
                        break;
                    }

                    case 0x0C: // formfeed
                    {
                        string_buffer[bytes++] = '\\';
                        string_buffer[bytes++] = 'f';
                        break;
                    }

                    case 0x0D: // carriage return
                    {
                        string_buffer[bytes++] = '\\';
                        string_buffer[bytes++] = 'r';
                        break;
                    }

                    case 0x22: // quotation mark
                    {
                        string_buffer[bytes++] = '&';
                        string_buffer[bytes++] = 'q';
                        string_buffer[bytes++] = 'u';
                        string_buffer[bytes++] = 'o';
                        string_buffer[bytes++] = 't';
                        string_buffer[bytes++] = ';';
                        break;
                    }

                    case 0x27: // apostrophe
                    {
                        string_buffer[bytes++] = '&';
                        string_buffer[bytes++] = 'a';
                        string_buffer[bytes++] = 'p';
                        string_buffer[bytes++] = 'o';
                        string_buffer[bytes++] = 's';
                        string_buffer[bytes++] = ';';
                        break;
                    }

                    case 0x26: // ampersand
                    {
                        string_buffer[bytes++] = '&';
                        string_buffer[bytes++] = 'a';
                        string_buffer[bytes++] = 'm';
                        string_buffer[bytes++] = 'p';
                        string_buffer[bytes++] = ';';
                        break;
                    }

                    case 0x3C: // less than
                    {
                        string_buffer[bytes++] = '\\';
                        string_buffer[bytes++] = 'l';
                        string_buffer[bytes++] = 't';
                        string_buffer[bytes++] = ';';
                        break;
                    }

                    case 0x3E: // greater than
                    {
                        string_buffer[bytes++] = '\\';
                        string_buffer[bytes++] = 'g';
                        string_buffer[bytes++] = 't';
                        string_buffer[bytes++] = ';';
                        break;
                    }

                    default:
                    {
                        // escape control characters (0x00..0x1F)
                        if ((codepoint <= 0x1F) or (codepoint >= 0x7F))
                        {
                            if (codepoint <= 0xFFFF)
                            {
                                (std::snprintf)(
                                    string_buffer.data() + bytes, 7, "\\u%04x",
                                    static_cast<uint16_t>(codepoint));
                                bytes += 6;
                            }
                            else
                            {
                                (std::snprintf)(
                                    string_buffer.data() + bytes, 13,
                                    "\\u%04x\\u%04x",
                                    static_cast<uint16_t>(0xD7C0 +
                                                          (codepoint >> 10)),
                                    static_cast<uint16_t>(0xDC00 +
                                                          (codepoint & 0x3FF)));
                                bytes += 12;
                            }
                        }
                        else
                        {
                            // copy byte to buffer (all previous bytes
                            // been copied have in default case above)
                            string_buffer[bytes++] = s[i];
                        }
                        break;
                    }
                }

                // write buffer and reset index; there must be 13 bytes
                // left, as this is the maximal number of bytes to be
                // written ("\uxxxx\uxxxx\0") for one code point
                if (string_buffer.size() - bytes < 13)
                {
                    o.append(string_buffer.data(), bytes);
                    bytes = 0;
                }

                // remember the byte position of this accept
                bytes_after_last_accept = bytes;
                undumped_chars = 0;
                break;
            }

            case UTF8_REJECT: // decode found invalid UTF-8 byte
            {
                // in case we saw this character the first time, we
                // would like to read it again, because the byte
                // may be OK for itself, but just not OK for the
                // previous sequence
                if (undumped_chars > 0)
                {
                    --i;
                }

                // reset length buffer to the last accepted index;
                // thus removing/ignoring the invalid characters
                bytes = bytes_after_last_accept;

                string_buffer[bytes++] = '\\';
                string_buffer[bytes++] = 'u';
                string_buffer[bytes++] = 'f';
                string_buffer[bytes++] = 'f';
                string_buffer[bytes++] = 'f';
                string_buffer[bytes++] = 'd';

                bytes_after_last_accept = bytes;

                undumped_chars = 0;

                // continue processing the string
                state = UTF8_ACCEPT;
                break;

                break;
            }

            default: // decode found yet incomplete multi-byte code point
            {
                ++undumped_chars;
                break;
            }
        }
    }

    // we finished processing the string
    if (state == UTF8_ACCEPT)
    {
        // write buffer
        if (bytes > 0)
        {
            o.append(string_buffer.data(), bytes);
        }
    }
    else
    {
        // write all accepted bytes
        o.append(string_buffer.data(), bytes_after_last_accept);
        o += "\\ufffd";
    }
}

inline unsigned int count_digits(uint64_t x) noexcept
{
    unsigned int n_digits = 1;
    for (;;)
    {
        if (x < 10)
        {
            return n_digits;
        }
        if (x < 100)
        {
            return n_digits + 1;
        }
        if (x < 1000)
        {
            return n_digits + 2;
        }
        if (x < 10000)
        {
            return n_digits + 3;
        }
        x = x / 10000u;
        n_digits += 4;
    }
}

template <typename NumberType,
          std::enable_if_t<std::is_same<NumberType, uint64_t>::value or
                               std::is_same<NumberType, int64_t>::value,
                           int> = 0>
void dump_integer(std::string& o, NumberType x)
{
    std::array<char, 64> number_buffer{{}};

    static constexpr std::array<std::array<char, 2>, 100> digits_to_99{{
        {'0', '0'}, {'0', '1'}, {'0', '2'}, {'0', '3'}, {'0', '4'}, {'0', '5'},
        {'0', '6'}, {'0', '7'}, {'0', '8'}, {'0', '9'}, {'1', '0'}, {'1', '1'},
        {'1', '2'}, {'1', '3'}, {'1', '4'}, {'1', '5'}, {'1', '6'}, {'1', '7'},
        {'1', '8'}, {'1', '9'}, {'2', '0'}, {'2', '1'}, {'2', '2'}, {'2', '3'},
        {'2', '4'}, {'2', '5'}, {'2', '6'}, {'2', '7'}, {'2', '8'}, {'2', '9'},
        {'3', '0'}, {'3', '1'}, {'3', '2'}, {'3', '3'}, {'3', '4'}, {'3', '5'},
        {'3', '6'}, {'3', '7'}, {'3', '8'}, {'3', '9'}, {'4', '0'}, {'4', '1'},
        {'4', '2'}, {'4', '3'}, {'4', '4'}, {'4', '5'}, {'4', '6'}, {'4', '7'},
        {'4', '8'}, {'4', '9'}, {'5', '0'}, {'5', '1'}, {'5', '2'}, {'5', '3'},
        {'5', '4'}, {'5', '5'}, {'5', '6'}, {'5', '7'}, {'5', '8'}, {'5', '9'},
        {'6', '0'}, {'6', '1'}, {'6', '2'}, {'6', '3'}, {'6', '4'}, {'6', '5'},
        {'6', '6'}, {'6', '7'}, {'6', '8'}, {'6', '9'}, {'7', '0'}, {'7', '1'},
        {'7', '2'}, {'7', '3'}, {'7', '4'}, {'7', '5'}, {'7', '6'}, {'7', '7'},
        {'7', '8'}, {'7', '9'}, {'8', '0'}, {'8', '1'}, {'8', '2'}, {'8', '3'},
        {'8', '4'}, {'8', '5'}, {'8', '6'}, {'8', '7'}, {'8', '8'}, {'8', '9'},
        {'9', '0'}, {'9', '1'}, {'9', '2'}, {'9', '3'}, {'9', '4'}, {'9', '5'},
        {'9', '6'}, {'9', '7'}, {'9', '8'}, {'9', '9'},
    }};

    // special case for "0"
    if (x == 0)
    {
        o += '0';
        return;
    }

    // use a pointer to fill the buffer
    auto buffer_ptr = begin(number_buffer);

    const bool is_negative = std::is_same<NumberType, int64_t>::value and
                             not(x >= 0); // see issue #755
    uint64_t abs_value;

    unsigned int n_chars;

    if (is_negative)
    {
        *buffer_ptr = '-';
        abs_value = static_cast<uint64_t>(0 - x);

        // account one more byte for the minus sign
        n_chars = 1 + count_digits(abs_value);
    }
    else
    {
        abs_value = static_cast<uint64_t>(x);
        n_chars = count_digits(abs_value);
    }

    // spare 1 byte for '\0'
    assert(n_chars < number_buffer.size() - 1);

    // jump to the end to generate the string from backward
    // so we later avoid reversing the result
    buffer_ptr += n_chars;

    // Fast int2ascii implementation inspired by "Fastware" talk by Andrei
    // Alexandrescu See: https://www.youtube.com/watch?v=o4-CwDo2zpg
    while (abs_value >= 100)
    {
        const auto digits_index = static_cast<unsigned>((abs_value % 100));
        abs_value /= 100;
        *(--buffer_ptr) = digits_to_99[digits_index][1];
        *(--buffer_ptr) = digits_to_99[digits_index][0];
    }

    if (abs_value >= 10)
    {
        const auto digits_index = static_cast<unsigned>(abs_value);
        *(--buffer_ptr) = digits_to_99[digits_index][1];
        *(--buffer_ptr) = digits_to_99[digits_index][0];
    }
    else
    {
        *(--buffer_ptr) = static_cast<char>('0' + abs_value);
    }

    o.append(number_buffer.data(), n_chars);
}

void dump_float(std::string& o, double x,
                std::true_type /*is_ieee_single_or_double*/)
{
    std::array<char, 64> number_buffer{{}};
    char* begin = number_buffer.data();
    ::nlohmann::detail::to_chars(begin, begin + number_buffer.size(), x);

    o += begin;
}

void dump_float(std::string& o, double x,
                std::false_type /*is_ieee_single_or_double*/)
{
    std::array<char, 64> number_buffer{{}};
    // get number of digits for a float -> text -> float round-trip
    static constexpr auto d = std::numeric_limits<double>::max_digits10;

    // the actual conversion
    std::ptrdiff_t len = (std::snprintf)(number_buffer.data(),
                                         number_buffer.size(), "%.*g", d, x);

    // negative value indicates an error
    assert(len > 0);
    // check if buffer was large enough
    assert(static_cast<std::size_t>(len) < number_buffer.size());

    const auto end =
        std::remove(number_buffer.begin(), number_buffer.begin() + len, ',');
    std::fill(end, number_buffer.end(), '\0');
    assert((end - number_buffer.begin()) <= len);
    len = (end - number_buffer.begin());

    o.append(number_buffer.data(), static_cast<std::size_t>(len));

    // determine if need to append ".0"
    const bool value_is_int_like =
        std::none_of(number_buffer.begin(), number_buffer.begin() + len + 1,
                     [](char c) { return (c == '.' or c == 'e'); });

    if (value_is_int_like)
    {
        o += ".0";
    }
}

void dump_float(std::string& o, double x)
{
    // NaN / inf
    if (not std::isfinite(x))
    {
        o += "null";
        return;
    }

    // If float is an IEEE-754 single or double precision number,
    // use the Grisu2 algorithm to produce short numbers which are
    // guaranteed to round-trip, using strtof and strtod, resp.
    //
    // NB: The test below works if <long double> == <double>.
    static constexpr bool is_ieee_single_or_double =
        (std::numeric_limits<double>::is_iec559 and
         std::numeric_limits<double>::digits == 24 and
         std::numeric_limits<double>::max_exponent == 128) or
        (std::numeric_limits<double>::is_iec559 and
         std::numeric_limits<double>::digits == 53 and
         std::numeric_limits<double>::max_exponent == 1024);

    dump_float(o, x, std::integral_constant<bool, is_ieee_single_or_double>());
}

void dump(std::string& o, const nlohmann::json& val,
          const unsigned int current_indent = 0)
{
    std::string indent_string;

    switch (val.type())
    {
        case nlohmann::json::value_t::object:
        {
            if (val.empty())
            {
                o += "{}";
                return;
            }

            o += "{<br>";

            // variable to hold indentation for recursive calls
            const unsigned int new_indent = current_indent + 12;
            while (indent_string.size() < new_indent)
            {
                indent_string += "&nbsp;";
            }

            for (auto i = val.begin(); i != val.end();)
            {
                o += indent_string.substr(0, new_indent);

                o += "&quot";
                dump_escaped(o, i.key());
                o += "&quot: ";

                bool inATag = false;
                if (i.key() == "@odata.id" || i.key() == "@odata.context" ||
                    i.key() == "Members@odata.nextLink" || i.key() == "Uri")
                {
                    inATag = true;
                    o += "<a href=\"";
                    dump_escaped(o, i.value());
                    o += "\">";
                }
                dump(o, i.value(), new_indent);
                if (inATag)
                {
                    o += "</a>";
                }
                i++;
                if (i == val.end())
                {
                    o += ",";
                }
                o += "<br>";
            }
            o += indent_string.substr(0, current_indent);
            o += '}';

            return;
        }

        case nlohmann::json::value_t::array:
        {
            if (val.empty())
            {
                o += "[]";
                return;
            }

            o += "[<br>";

            // variable to hold indentation for recursive calls
            const auto new_indent = current_indent + 12;
            while (indent_string.size() < new_indent)
            {
                indent_string += "&nbsp;";
            }

            // first n-1 elements
            for (auto i = val.cbegin(); i != val.cend() - 1; ++i)
            {
                o += indent_string.c_str();
                dump(o, *i, new_indent);
                o += ",<br>";
            }

            // last element
            assert(not val.empty());
            o += indent_string.c_str();
            dump(o, val.back(), new_indent);

            o += "<br>";
            o += indent_string.c_str();
            o += ']';

            return;
        }

        case nlohmann::json::value_t::string:
        {
            o += '\"';
            const std::string* ptr = val.get_ptr<const std::string*>();
            dump_escaped(o, *ptr);
            o += '\"';
            return;
        }

        case nlohmann::json::value_t::boolean:
        {
            if (*(val.get_ptr<const bool*>()))
            {
                o += "true";
            }
            else
            {
                o += "false";
            }
            return;
        }

        case nlohmann::json::value_t::number_integer:
        {
            dump_integer(o, *(val.get_ptr<const int64_t*>()));
            return;
        }

        case nlohmann::json::value_t::number_unsigned:
        {
            dump_integer(o, *(val.get_ptr<const uint64_t*>()));
            return;
        }

        case nlohmann::json::value_t::number_float:
        {
            dump_float(o, *(val.get_ptr<const double*>()));
            return;
        }

        case nlohmann::json::value_t::discarded:
        {
            o += "<discarded>";
            return;
        }

        case nlohmann::json::value_t::null:
        {
            o += "null";
            return;
        }
    }
}

void dump_html(std::string& o, const nlohmann::json& j)
{
    o += "<html>\n"
         "<head>\n"
         "<title>Redfish API</title>\n"
         "</head>\n"
         "<body>\n"
         "<div style=\"max-width: 576px;margin:0 auto;\">\n"
         "<img src=\"/DMTF_Redfish_logo_2017.svg\" alt=\"redfish\" "
         "height=\"406px\" "
         "width=\"576px\">\n"
         "<br>\n"
         "<div>\n";
    dump(o, j);
    o += "</div>\n"
         "</div>\n"
         "</body>\n"
         "</html>\n";
}

} // namespace json_html_util
