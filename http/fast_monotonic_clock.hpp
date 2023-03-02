#pragma once

#include <chrono>

class fast_monotonic_clock
{
  public:
    using duration = std::chrono::nanoseconds;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = std::chrono::time_point<fast_monotonic_clock>;

    static constexpr bool is_steady = true;

    static time_point now() noexcept
    {
        struct timespec t;
        const auto result = clock_gettime(CLOCK_MONOTONIC_COARSE, &t);
        assert(result == 0);
        return time_point{convert(t)};
    }

    static duration get_resolution() noexcept
    {
        struct timespec t;
        clock_getres(CLOCK_MONOTONIC_COARSE, &t);
        return convert(t);
    }

  private:
    static duration convert(const timespec& t)
    {
        return std::chrono::seconds(t.tv_sec) +
               std::chrono::nanoseconds(t.tv_nsec);
    }
};
