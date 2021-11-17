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
            finalizer();
        }
    }

  private:
    std::function<void()> finalizer;
};

} // namespace utils

} // namespace redfish