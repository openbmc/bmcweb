#include "aggregation_utils.hpp"
#include "app.hpp"
#include "redfish.hpp"

#include <nlohmann/json.hpp>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{
inline bool uriMatchesPath(std::string_view uri, std::string_view path)
{
    size_t uriIndex = 0;
    size_t pathIndex = 0;
    while (uriIndex < uri.size() && pathIndex < path.size())
    {
        // If characters match, move onto next char
        if (uri[uriIndex] == path[pathIndex])
        {
            ++uriIndex;
            ++pathIndex;
            continue;
        }
        // If path has a wildcard, denoted by '[*]', then fast forward to next
        // '/'
        if (path[pathIndex] == '[')
        {
            while (uriIndex < uri.size() && uri[uriIndex] != '/')
            {
                ++uriIndex;
            }
            while (pathIndex < path.size() && path[pathIndex] != '/')
            {
                ++pathIndex;
            }
            continue;
        }
        // If neither above is true, then return false;
        return false;
    }
    return true;
}

inline std::string_view findEntityType(std::string_view uri)
{
    for (const Path& entityType : topCollections)
    {
        if (uriMatchesPath(uri, entityType.path))
        {
            return entityType.type;
        }
    }
    return "";
}

TEST(EntityUriRegistryTest, EnsureAllBmcwebRoutesAreInRegistry)
{
    crow::App app;
    // Add all redfish handlers
    redfish::RedfishService redfish(app);

    app.validate();

    for (const auto* const route : app.getRoutes())
    {
        EXPECT_NE(findEntityType(*route), "");
    }
}

} // namespace
} // namespace redfish
