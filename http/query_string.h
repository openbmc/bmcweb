#pragma once

#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace crow
{
// ----------------------------------------------------------------------------
// qs_parse (modified)
// https://github.com/bartgrantham/qs_parse
// ----------------------------------------------------------------------------
/*  Similar to strncmp, but handles URL-encoding for either string  */
int qsStrncmp(const char* s, const char* qs, size_t n);

/*  Finds the beginning of each key/value pair and stores a pointer in qs_kv.
 *  Also decodes the value portion of the k/v pair *in-place*.  In a future
 *  enhancement it will also have a compile-time option of sorting qs_kv
 *  alphabetically by key.  */
size_t qsParse(char* qs, char* qs_kv[], size_t qs_kv_size);

/*  Used by qs_parse to decode the value portion of a k/v pair  */
int qsDecode(char* qs);

/*  Looks up the value according to the key on a pre-processed query string
 *  A future enhancement will be a compile-time option to look up the key
 *  in a pre-sorted qs_kv array via a binary search.  */
// char * qs_k2v(const char * key, char * qs_kv[], int qs_kv_size);
char* qsK2v(const char* key, char* const* qs_kv, int qs_kv_size, int nth);

/*  Non-destructive lookup of value, based on key.  User provides the
 *  destinaton string and length.  */
char* qsScanvalue(const char* key, const char* qs, char* val, size_t val_len);

// TODO: implement sorting of the qs_kv array; for now ensure it's not compiled
#undef _qsSORTING

// isxdigit _is_ available in <ctype.h>, but let's avoid another header instead
#define BMCWEB_QS_ISHEX(x)                                                     \
    ((((x) >= '0' && (x) <= '9') || ((x) >= 'A' && (x) <= 'F') ||              \
      ((x) >= 'a' && (x) <= 'f'))                                              \
         ? 1                                                                   \
         : 0)
#define BMCWEB_QS_HEX2DEC(x)                                                   \
    (((x) >= '0' && (x) <= '9')                                                \
         ? (x)-48                                                              \
         : ((x) >= 'A' && (x) <= 'F')                                          \
               ? (x)-55                                                        \
               : ((x) >= 'a' && (x) <= 'f') ? (x)-87 : 0)
#define BMCWEB_QS_ISQSCHR(x)                                                   \
    ((((x) == '=') || ((x) == '#') || ((x) == '&') || ((x) == '\0')) ? 0 : 1)

inline int qsStrncmp(const char* s, const char* qs, size_t n)
{
    int i = 0;
    char u1, u2;
    char unyb, lnyb;

    while (n-- > 0)
    {
        u1 = *s++;
        u2 = *qs++;

        if (!BMCWEB_QS_ISQSCHR(u1))
        {
            u1 = '\0';
        }
        if (!BMCWEB_QS_ISQSCHR(u2))
        {
            u2 = '\0';
        }

        if (u1 == '+')
        {
            u1 = ' ';
        }
        if (u1 == '%') // easier/safer than scanf
        {
            unyb = static_cast<char>(*s++);
            lnyb = static_cast<char>(*s++);
            if (BMCWEB_QS_ISHEX(unyb) && BMCWEB_QS_ISHEX(lnyb))
            {
                u1 = static_cast<char>((BMCWEB_QS_HEX2DEC(unyb) * 16) +
                                       BMCWEB_QS_HEX2DEC(lnyb));
            }
            else
            {
                u1 = '\0';
            }
        }

        if (u2 == '+')
        {
            u2 = ' ';
        }
        if (u2 == '%') // easier/safer than scanf
        {
            unyb = static_cast<char>(*qs++);
            lnyb = static_cast<char>(*qs++);
            if (BMCWEB_QS_ISHEX(unyb) && BMCWEB_QS_ISHEX(lnyb))
            {
                u2 = static_cast<char>((BMCWEB_QS_HEX2DEC(unyb) * 16) +
                                       BMCWEB_QS_HEX2DEC(lnyb));
            }
            else
            {
                u2 = '\0';
            }
        }

        if (u1 != u2)
        {
            return u1 - u2;
        }
        if (u1 == '\0')
        {
            return 0;
        }
        i++;
    }
    if (BMCWEB_QS_ISQSCHR(*qs))
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

inline size_t qsParse(char* qs, char* qs_kv[], size_t qs_kv_size)
{
    size_t i;
    size_t j;
    char* substrPtr;

    for (i = 0; i < qs_kv_size; i++)
    {
        qs_kv[i] = nullptr;
    }

    // find the beginning of the k/v substrings or the fragment
    substrPtr = qs + strcspn(qs, "?#");
    if (substrPtr[0] != '\0')
    {
        substrPtr++;
    }
    else
    {
        return 0; // no query or fragment
    }

    i = 0;
    while (i < qs_kv_size)
    {
        qs_kv[i] = substrPtr;
        j = strcspn(substrPtr, "&");
        if (substrPtr[j] == '\0')
        {
            break;
        }
        substrPtr += j + 1;
        i++;
    }
    i++; // x &'s -> means x iterations of this loop -> means *x+1* k/v pairs

    // we only decode the values in place, the keys could have '='s in them
    // which will hose our ability to distinguish keys from values later
    for (j = 0; j < i; j++)
    {
        substrPtr = qs_kv[j] + strcspn(qs_kv[j], "=&#");
        if (substrPtr[0] == '&' || substrPtr[0] == '\0')
        { // blank value: skip decoding
            substrPtr[0] = '\0';
        }
        else
        {
            qsDecode(++substrPtr);
        }
    }

#ifdef _qsSORTING
// TODO: qsort qs_kv, using qs_strncmp() for the comparison
#endif

    return i;
}

inline int qsDecode(char* qs)
{
    int i = 0, j = 0;

    while (BMCWEB_QS_ISQSCHR(qs[j]))
    {
        if (qs[j] == '+')
        {
            qs[i] = ' ';
        }
        else if (qs[j] == '%') // easier/safer than scanf
        {
            if (!BMCWEB_QS_ISHEX(qs[j + 1]) || !BMCWEB_QS_ISHEX(qs[j + 2]))
            {
                qs[i] = '\0';
                return i;
            }
            qs[i] = static_cast<char>(BMCWEB_QS_HEX2DEC(qs[j + 1] * 16) +
                                      BMCWEB_QS_HEX2DEC(qs[j + 2]));
            j += 2;
        }
        else
        {
            qs[i] = qs[j];
        }
        i++;
        j++;
    }
    qs[i] = '\0';

    return i;
}

inline char* qsK2v(const char* key, char* const* qs_kv, int qs_kv_size,
                   int nth = 0)
{
    int i;
    size_t keyLen, skip;

    keyLen = strlen(key);

#ifdef _qsSORTING
// TODO: binary search for key in the sorted qs_kv
#else  // _qsSORTING
    for (i = 0; i < qs_kv_size; i++)
    {
        // we rely on the unambiguous '=' to find the value in our k/v pair
        if (qsStrncmp(key, qs_kv[i], keyLen) == 0)
        {
            skip = strcspn(qs_kv[i], "=");
            if (qs_kv[i][skip] == '=')
            {
                skip++;
            }
            // return (zero-char value) ? ptr to trailing '\0' : ptr to value
            if (nth == 0)
            {
                return qs_kv[i] + skip;
            }
            else
            {
                --nth;
            }
        }
    }
#endif // _qsSORTING

    return nullptr;
}

inline char* qsScanvalue(const char* key, const char* qs, char* val,
                         size_t val_len)
{
    size_t i, keyLen;
    const char* tmp;

    // find the beginning of the k/v substrings
    if ((tmp = strchr(qs, '?')) != nullptr)
    {
        qs = tmp + 1;
    }

    keyLen = strlen(key);
    while (qs[0] != '#' && qs[0] != '\0')
    {
        if (qsStrncmp(key, qs, keyLen) == 0)
        {
            break;
        }
        qs += strcspn(qs, "&") + 1;
    }

    if (qs[0] == '\0')
    {
        return nullptr;
    }

    qs += strcspn(qs, "=&#");
    if (qs[0] == '=')
    {
        qs++;
        i = strcspn(qs, "&=#");
        strncpy(val, qs, (val_len - 1) < (i + 1) ? (val_len - 1) : (i + 1));
        qsDecode(val);
    }
    else
    {
        if (val_len > 0)
        {
            val[0] = '\0';
        }
    }

    return val;
}
} // namespace crow
// ----------------------------------------------------------------------------

namespace crow
{
class QueryString
{
  public:
    static const size_t maxKeyValuePairsCount = 256;

    QueryString() = default;

    QueryString(const QueryString& qs) : url(qs.url)
    {
        for (auto p : qs.keyValuePairs)
        {
            keyValuePairs.push_back(
                const_cast<char*>(p - qs.url.c_str() + url.c_str()));
        }
    }

    QueryString& operator=(const QueryString& qs)
    {
        if (this == &qs)
        {
            return *this;
        }

        url = qs.url;
        keyValuePairs.clear();
        for (auto p : qs.keyValuePairs)
        {
            keyValuePairs.push_back(
                const_cast<char*>(p - qs.url.c_str() + url.c_str()));
        }
        return *this;
    }

    QueryString& operator=(QueryString&& qs)
    {
        keyValuePairs = std::move(qs.keyValuePairs);
        auto* oldData = const_cast<char*>(qs.url.c_str());
        url = std::move(qs.url);
        for (auto& p : keyValuePairs)
        {
            p += const_cast<char*>(url.c_str()) - oldData;
        }
        return *this;
    }

    explicit QueryString(std::string newUrl) : url(std::move(newUrl))
    {
        if (url.empty())
        {
            return;
        }

        keyValuePairs.resize(maxKeyValuePairsCount);

        size_t count =
            qsParse(&url[0], &keyValuePairs[0], maxKeyValuePairsCount);
        keyValuePairs.resize(count);
    }

    void clear()
    {
        keyValuePairs.clear();
        url.clear();
    }

    friend std::ostream& operator<<(std::ostream& os, const QueryString& qs)
    {
        os << "[ ";
        for (size_t i = 0; i < qs.keyValuePairs.size(); ++i)
        {
            if (i != 0u)
            {
                os << ", ";
            }
            os << qs.keyValuePairs[i];
        }
        os << " ]";
        return os;
    }

    char* get(const std::string& name) const
    {
        char* ret = qsK2v(name.c_str(), keyValuePairs.data(),
                          static_cast<int>(keyValuePairs.size()));
        return ret;
    }

    std::vector<char*> getList(const std::string& name) const
    {
        std::vector<char*> ret;
        std::string plus = name + "[]";
        char* element = nullptr;

        int count = 0;
        while (true)
        {
            element = qsK2v(plus.c_str(), keyValuePairs.data(),
                            static_cast<int>(keyValuePairs.size()), count++);
            if (element == nullptr)
            {
                break;
            }
            ret.push_back(element);
        }
        return ret;
    }

  private:
    std::string url;
    std::vector<char*> keyValuePairs;
};

} // namespace crow
