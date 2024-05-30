#include "app.hpp"
#include "redfish.hpp"

#include <boost/asio/io_context.hpp>

#include <memory>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(Redfish, PathsShouldValidate)
{
    auto io = std::make_shared<boost::asio::io_context>();
    crow::App app(io);

    RedfishService redfish(app);

    app.validate();
}

} // namespace
} // namespace redfish
