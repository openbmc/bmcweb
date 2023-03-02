#include "fast_monotonic_clock.hpp"

#include <gtest/gtest.h> // IWYU pragma: keep


TEST(FastMonotonicClock, BasicUsage)
{
  static_assert(fast_monotonic_clock::is_steady);

  fast_monotonic_clock clock;
  
  EXPECT_GT(clock.now().time_since_epoch().count(), 0);
  EXPECT_EQ(clock.get_resolution(), std::chrono::milliseconds(4));
}

