#pragma once

#include <boost/circular_buffer.hpp>
#include <boost/circular_buffer/space_optimized.hpp>
#include <chrono>
#include <functional>

#include "logging.h"

namespace crow
{
namespace detail
{
// fast timer queue for fixed tick value.
class TimerQueue
{
  public:
    TimerQueue()
    {
        dq.set_capacity(100);
    }

    void cancel(size_t k)
    {
        size_t index = k - step;
        if (index < dq.size())
        {
            dq[index].second = nullptr;
        }
    }

    size_t add(std::function<void()> f)
    {
        dq.push_back(
            std::make_pair(std::chrono::steady_clock::now(), std::move(f)));
        size_t ret = step + dq.size() - 1;

        BMCWEB_LOG_DEBUG << "timer add inside: " << this << ' ' << ret;
        return ret;
    }

    void process()
    {
        auto now = std::chrono::steady_clock::now();
        while (!dq.empty())
        {
            auto& x = dq.front();
            // Check expiration time only for active handlers,
            // remove canceled ones immediately
            if (x.second)
            {
                if (now - x.first < std::chrono::seconds(5))
                {
                    break;
                }

                BMCWEB_LOG_DEBUG << "timer call: " << this << ' ' << step;
                // we know that timer handlers are very simple currenty; call
                // here
                x.second();
            }
            dq.pop_front();
            step++;
        }
    }

  private:
    using storage_type =
        std::pair<std::chrono::time_point<std::chrono::steady_clock>,
                  std::function<void()>>;

    boost::circular_buffer_space_optimized<storage_type,
                                           std::allocator<storage_type>>
        dq{};

    // boost::circular_buffer<storage_type> dq{20};
    // std::deque<storage_type> dq{};
    size_t step{};
};
} // namespace detail
} // namespace crow
