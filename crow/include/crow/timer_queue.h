#pragma once

#include <chrono>
#include <functional>
#include "crow/logging.h"
#include <boost/circular_buffer.hpp>
#include <boost/circular_buffer/space_optimized.hpp>

namespace crow {
namespace detail {
// fast timer queue for fixed tick value.
class timer_queue {
 public:
  timer_queue() { dq_.set_capacity(100); }

  void cancel(int k) {
    unsigned int index = static_cast<unsigned int>(k - step_);
    if (index < dq_.size()) {
      dq_[index].second = nullptr;
    }
  }

  int add(std::function<void()> f) {
    dq_.push_back(
        std::make_pair(std::chrono::steady_clock::now(), std::move(f)));
    int ret = step_ + dq_.size() - 1;

    CROW_LOG_DEBUG << "timer add inside: " << this << ' ' << ret;
    return ret;
  }

  void process() {
    auto now = std::chrono::steady_clock::now();
    while (!dq_.empty()) {
      auto& x = dq_.front();
      if (now - x.first < std::chrono::seconds(5)) {
        break;
      }
      if (x.second) {
        CROW_LOG_DEBUG << "timer call: " << this << ' ' << step_;
        // we know that timer handlers are very simple currenty; call here
        x.second();
      }
      dq_.pop_front();
      step_++;
    }
  }

 private:
  using storage_type =
      std::pair<std::chrono::time_point<std::chrono::steady_clock>,
                std::function<void()>>;

  boost::circular_buffer_space_optimized<storage_type,
                                         std::allocator<storage_type>>
      dq_{};

  // boost::circular_buffer<storage_type> dq_{20};
  // std::deque<storage_type> dq_{};
  int step_{};
};
}  // namespace detail
}  // namespace crow
