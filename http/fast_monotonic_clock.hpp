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

    static time_point now() noexcept;

    static duration get_resolution() noexcept;

  private:
    static clockid_t clock_id();
    static clockid_t test_coarse_clock();
    static duration convert(const timespec&);
};

inline auto fast_monotonic_clock::convert(const timespec& t) -> duration
{
    return std::chrono::seconds(t.tv_sec) + std::chrono::nanoseconds(t.tv_nsec);
}

auto fast_monotonic_clock::now() noexcept -> time_point
{
    struct timespec t;
    const auto result = clock_gettime(CLOCK_MONOTONIC_COARSE, &t);
    assert(result == 0);
    return time_point{convert(t)};
}

auto fast_monotonic_clock::get_resolution() noexcept -> duration
{
    struct timespec t;
    const auto result = clock_getres(CLOCK_MONOTONIC_COARSE, &t);
    return convert(t);
}
