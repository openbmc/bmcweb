#pragma once

#include <functional>

namespace redfish
{

namespace utils
{

class Finalizer
{
  public:
    Finalizer() = delete;
    Finalizer(std::function<void()> finalizer) : finalizer(std::move(finalizer))
    {}

    Finalizer(const Finalizer&) = delete;
    Finalizer(Finalizer&&) = delete;

    ~Finalizer()
    {
        if (finalizer)
        {
            try
            {
                finalizer();
            }
            catch (const std::exception& e)
            {
                BMCWEB_LOG_ERROR << "Executing finalizer failed: " << e.what();
            }
        }
    }

  private:
    std::function<void()> finalizer;
};

} // namespace utils

} // namespace redfish
