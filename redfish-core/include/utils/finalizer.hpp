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
    Finalizer(std::function<void()> finalizerArg) :
        finalizer(std::move(finalizerArg))
    {}

    Finalizer(const Finalizer&) = delete;
    Finalizer(Finalizer&&) = delete;

    Finalizer& operator=(const Finalizer&) = delete;
    Finalizer& operator=(Finalizer&&) = delete;

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
