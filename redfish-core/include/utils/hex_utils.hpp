#pragma once

#include <array>
#include <cstddef>
#include <string>

namespace details
{
inline std::string intToHexStringHelper(uint64_t value, size_t digits)
{
    static constexpr std::array<char, 16> digitsArray = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    std::string rc(digits, '0');
    size_t bitIndex = (digits - 1) * 4;
    for (size_t digitIndex = 0; digitIndex < digits; digitIndex++)
    {
        rc[digitIndex] = digitsArray[(value >> bitIndex) & 0x0f];
        bitIndex -= 4;
    }
    return rc;
}
} // namespace details

template <typename IntegerType>
inline std::string intToHexString(IntegerType value,
                                  size_t digits = sizeof(IntegerType) << 1)
{
    static_assert(sizeof(IntegerType) <= sizeof(uint64_t),
                  "Integer size too big");
    static_assert(std::is_unsigned<IntegerType>::value,
                  "Integer must be unsigned");
    return details::intToHexStringHelper(value, digits);
}
