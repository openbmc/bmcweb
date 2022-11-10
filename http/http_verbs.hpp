#pragma once

#include <boost/beast/http/verb.hpp>

namespace crow
{

// Note, this is an imperfect abstraction.  There are a lot of verbs that we
// use memory for, but are basically unused by most implementations.
// Ideally we would have a list of verbs that we do use, and only index in
// to a smaller array of those, but that would require a translation from
// boost::beast::http::verb, to the bmcweb index.
inline constexpr size_t maxVerbIndex =
    static_cast<size_t>(boost::beast::http::verb::patch);

// MaxVerb + 1 is designated as the "not found" verb.  It is done this way
// to keep the BaseRule as a single bitfield (thus keeping the struct small)
// while still having a way to declare a route a "not found" route.
inline constexpr size_t notFoundIndex = maxVerbIndex + 1;
inline constexpr size_t methodNotAllowedIndex = notFoundIndex + 1;
inline constexpr size_t maxNumMethods = methodNotAllowedIndex + 1;

} // namespace crow