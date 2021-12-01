#pragma once

// As of clang-12, clang still doesn't support std::source_location, which
// means that clang-tidy also doesn't support std::source_location.
// Inside the libstdc++ implementation of <source_location> is this check of
// __builtin_source_location to determine if the compiler supports the
// necessary bits for std::source_location, and if not the header ends up doing
// nothing.  Use this same builtin-check to detect when we're running under
// an "older" clang and fallback to std::experimental::source_location instead.

#if __has_builtin(__builtin_source_location)
#include <source_location>

namespace bmcweb
{
using source_location = std::source_location;
}

#else
#include <experimental/source_location>

namespace bmcweb
{
using source_location = std::experimental::source_location;
}

#endif
