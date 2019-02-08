// This file was sourced from http://www.davekoelle.com/files/alphanum.hpp
// Modified to use bmcweb logging mechanisms, and removed asserts

#ifndef ALPHANUM__HPP
#define ALPHANUM__HPP

/*
 Released under the MIT License - https://opensource.org/licenses/MIT

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the "Software"),
 to deal in the Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/* $Header: /code/doj/alphanum.hpp,v 1.3 2008/01/28 23:06:47 doj Exp $ */

#include <crow/logging.h>

#include <functional>
#include <sstream>
#include <string>
#include <string_view>

// TODO: make comparison with hexadecimal numbers. Extend the alphanum_comp()
// function by traits to choose between decimal and hexadecimal.

namespace doj
{

/** this function does not consider the current locale and only
works with ASCII digits.
@return true if c is a digit character
*/
bool alphanum_isdigit(const char c)
{
    return c >= '0' && c <= '9';
}

/**
   compare l and r with strcmp() semantics, but using
   the "Alphanum Algorithm". This function is designed to read
   through the l and r strings only one time, for
   maximum performance. It does not allocate memory for
   substrings. It can either use the C-library functions isdigit()
   and atoi() to honour your locale settings, when recognizing
   digit characters when you "#define ALPHANUM_LOCALE=1" or use
   it's own digit character handling which only works with ASCII
   digit characters, but provides better performance.

   @param l NULL-terminated C-style string
   @param r NULL-terminated C-style string
   @return negative if l<r, 0 if l equals r, positive if l>r
 */
int alphanum_comp(const std::string_view l, const std::string_view r)
{
    enum mode_t
    {
        STRING,
        NUMBER
    } mode = STRING;

    std::string_view::iterator lc = l.begin();
    std::string_view::iterator rc = r.begin();

    while (lc != l.end() && rc != r.end())
    {
        if (mode == STRING)
        {
            char l_char, r_char;
            while ((l_char = *lc) && (r_char = *rc))
            {
                // check if this are digit characters
                const bool l_digit = alphanum_isdigit(l_char),
                           r_digit = alphanum_isdigit(r_char);
                // if both characters are digits, we continue in NUMBER mode
                if (l_digit && r_digit)
                {
                    mode = NUMBER;
                    break;
                }
                // if only the left character is a digit, we have a result
                if (l_digit)
                    return -1;
                // if only the right character is a digit, we have a result
                if (r_digit)
                    return +1;
                // compute the difference of both characters
                const int diff = l_char - r_char;
                // if they differ we have a result
                if (diff != 0)
                    return diff;
                // otherwise process the next characters
                ++lc;
                ++rc;
            }
        }
        else // mode==NUMBER
        {
            // get the left number
            unsigned long l_int = 0;
            while (*lc && alphanum_isdigit(*lc))
            {
                // TODO: this can overflow
                l_int = l_int * 10 + *lc - '0';
                ++lc;
            }

            // get the right number
            unsigned long r_int = 0;
            while (*rc && alphanum_isdigit(*rc))
            {
                // TODO: this can overflow
                r_int = r_int * 10 + *rc - '0';
                ++rc;
            }

            // if the difference is not equal to zero, we have a comparison
            // result
            const long diff = l_int - r_int;
            if (diff != 0)
                return diff;

            // otherwise we process the next substring in STRING mode
            mode = STRING;
        }
    }

    if (rc == r.end())
        return +1;
    if (lc == l.end())
        return -1;
    return 0;
}

/**
   Functor class to compare two objects with the "Alphanum
   Algorithm".
*/
struct alphanum_less
{
    bool operator()(const std::string_view left, const std::string_view right) const
    {
        return alphanum_comp(left, right) < 0;
    }
};

} // namespace doj

#endif