#include "app.hpp"

#include "redfish.hpp"

#include <nlohmann/json.hpp>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(EntityUriRegistryTest, EnsureAllBmcwebRoutesAreInRegistry){
    crow::App app;
    // Add all redfish handlers
    redfish::RedfishService redfish(app);

  for(const auto route : app.getRoutes()){
    std::cout << *route << std::endl;
  }
}

}
}