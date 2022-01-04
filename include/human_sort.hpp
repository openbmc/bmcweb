#pragma once

#include <string_view>

namespace details
{

// This implementation avoids the complexity of using std::isdigit, which pulls
// in all of <locale>, and likely has other consequences.
inline bool simpleIsDigit(const char c)
{
    return c >= '0' && c <= '9';
}

} // namespace details

inline int alphanumComp(const std::string_view left,
                        const std::string_view right)
{
    enum class ModeType
    {
        STRING,
        NUMBER
    } mode = ModeType::STRING;

    std::string_view::const_iterator l = left.begin();
    std::string_view::const_iterator r = right.begin();

    while (l != left.end() && r != right.end())
    {
        if (mode == ModeType::STRING)
        {
            while (l != left.end() && r != right.end())
            {
                // check if this are digit characters
                const bool lDigit = details::simpleIsDigit(*l);
                const bool rDigit = details::simpleIsDigit(*r);
                // if both characters are digits, we continue in NUMBER mode
                if (lDigit && rDigit)
                {
                    mode = ModeType::NUMBER;
                    break;
                }
                // if only the left character is a digit, we have a result
                if (lDigit)
                {
                    return -1;
                } // if only the right character is a digit, we have a result
                if (rDigit)
                {
                    return +1;
                }
                // compute the difference of both characters
                const int diff = *l - *r;
                // if they differ we have a result
                if (diff != 0)
                {
                    return diff;
                }
                // otherwise process the next characters
                l++;
                r++;
            }
        }
        else // mode==NUMBER
        {
            // get the left number
            int lInt = 0;
            while (l != left.end() && details::simpleIsDigit(*l))
            {
                lInt = lInt * 10 + static_cast<int>(*l) - '0';
                ++l;
            }

            // get the right number
            int rInt = 0;
            while (r != right.end() && details::simpleIsDigit(*r))
            {
                rInt = rInt * 10 + static_cast<int>(*r) - '0';
                ++r;
            }

            // if the difference is not equal to zero, we have a comparison
            // result
            const int diff = lInt - rInt;
            if (diff != 0)
            {
                return diff;
            }

            // otherwise we process the next substring in STRING mode
            mode = ModeType::STRING;
        }
    }
    if (r == right.end() && l == left.end())
    {
        return 0;
    }
    if (r == right.end())
    {
        return 1;
    }
    return -1;
}

// A generic template type compatible with std::less that can be used on generic
// containers (set, map, ect)
template <class Type>
struct AlphanumLess
{
    bool operator()(const Type& left, const Type& right) const
    {
        return alphanumComp(left, right) < 0;
    }
};
