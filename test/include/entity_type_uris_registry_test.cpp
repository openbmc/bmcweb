#include "app.hpp"
#include "redfish.hpp"
#include "registries/entity_type_uris_registry.hpp"

#include <nlohmann/json.hpp>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(EntityUriRegistryTest, EnsureAllBmcwebRoutesAreInRegistry)
{
    crow::App app;
    // Add all redfish handlers
    redfish::RedfishService redfish(app);

    app.validate();

    for (const auto* const route : app.getRoutes())
    {
        EXPECT_NE(findEntityType(*route), "");
        if (findEntityType(*route).empty())
        {
            std::cerr << *route << " is not in registry" << std::endl;
        }
    }
}

} // namespace
} // namespace redfish