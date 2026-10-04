// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
//
// Proves the difference between:
//
//   UNSAFE: bind_front(&Self::cb, this, weak_from_this())
//   SAFE:   lambda capturing weak_from_this(), body does weak.lock()
//
// --- The problem ---
//
// The timer pattern used in http_connection.hpp and
// server_sent_event_impl.hpp before this fix was:
//
//   UNSAFE:
//     timer.async_wait(
//         std::bind_front(&Self::onTimeout, this, weak_from_this()));
//
// std::bind_front stores its FIRST argument as the call target.  Here
// that is the raw `this` pointer.  weak_from_this() is bound as the
// first method argument — it does NOT keep the object alive.
//
// If all shared_ptr owners drop while the timer is still pending, the
// object is destroyed and `this` becomes a dangling pointer.  When the
// timer fires, the member function dispatch dereferences the dangling
// raw pointer to reach onTimeout's body — undefined behavior BEFORE
// the weak.lock() guard inside the body even runs.
//
// The safe replacement is a lambda that captures only the weak_ptr:
//
//   SAFE:
//     std::weak_ptr<Self> weak = weak_from_this();
//     timer.async_wait([weak](const boost::system::error_code& ec) {
//         if (auto self = weak.lock()) { self->onTimeout(ec); }
//     });
//
// The lambda holds no raw pointer.  The weak_ptr extends to nothing by
// itself, so the object can be destroyed normally.  When the timer
// fires, weak.lock() returns null and the body is skipped safely.
//
// --- Tests ---
//
//  1. SafeTimer_LambdaWeakPtr
//       Lambda captures weak_ptr.  Object is destroyed before the timer
//       fires.  lock() returns null; callback body is skipped — no UB.
//
//  2. UnsafeTimer_RawPtrDanglesAfterWeakExpires
//       Demonstrates that bind_front(&Self::cb, this, weak_from_this())
//       makes `this` the call target with no ownership.  Destroying the
//       object before the handler runs makes `this` dangling.  Any
//       dereference through it is UB; the test captures the invariant
//       without actually performing the unsafe dereference.
//
//  3. SafeTimer_CallbackInvokedWhenAlive
//       Lambda captures weak_ptr.  Object is still alive when the timer
//       fires.  lock() succeeds; the callback body runs correctly.

#include <functional>
#include <memory>
#include <queue>
#include <utility>

#include <gtest/gtest.h>

namespace
{

// ============================================================================
// IoQueue — same minimal model as bind_front_this_test.cpp.
// ============================================================================
struct IoQueue
{
    void post(std::function<void()> handler)
    {
        queue.push(std::move(handler));
    }

    void drain()
    {
        while (!queue.empty())
        {
            std::function<void()> handler = std::move(queue.front());
            queue.pop();
            handler();
        }
    }

    std::queue<std::function<void()>> queue;
};

// ============================================================================
// TimerWorker — models an async connection object with a deadline timer.
//
// onTimeout() models afterTimerWait / onTimeoutCallback: it first does a
// weak.lock() check, then acts on the live object.  The safe dispatch
// pattern never calls onTimeout() with a dangling `this`.
// ============================================================================
struct TimerWorker : std::enable_shared_from_this<TimerWorker>
{
    bool& destroyed;
    int timeoutCount = 0;
    bool bodyReached = false;

    explicit TimerWorker(bool& destroyedFlag) : destroyed(destroyedFlag) {}
    TimerWorker(const TimerWorker&) = delete;
    TimerWorker& operator=(const TimerWorker&) = delete;
    TimerWorker(TimerWorker&&) = delete;
    TimerWorker& operator=(TimerWorker&&) = delete;
    ~TimerWorker()
    {
        destroyed = true;
    }

    // Safe callback: no raw `this` involvement in dispatch.
    // Equivalent to what afterTimerWait / onTimeoutCallback do when reached
    // through the safe lambda path.
    void onTimeout()
    {
        bodyReached = true;
        timeoutCount++;
    }

    // -----------------------------------------------------------------------
    // Safe dispatch: post a lambda that holds only a weak_ptr.
    // The object may or may not be alive when the timer fires.
    // -----------------------------------------------------------------------
    void scheduleTimerSafe(IoQueue& q)
    {
        std::weak_ptr<TimerWorker> weak = weak_from_this();
        q.post([weak]() {
            std::shared_ptr<TimerWorker> self = weak.lock();
            if (self)
            {
                self->onTimeout();
            }
        });
    }

    // -----------------------------------------------------------------------
    // Unsafe dispatch: bind_front stores raw `this` as call target;
    // weak_from_this() is a bound argument (NOT a lifetime guardian).
    //
    // This models the pre-fix pattern:
    //   bind_front(&TimerWorker::unsafeOnTimeout, this, weak_from_this())
    // -----------------------------------------------------------------------
    void unsafeOnTimeout(const std::weak_ptr<TimerWorker>& /*weak*/)
    {
        // In production this would be: weak.lock(); if (!self) return; ...
        // But `this` was already dereferenced to get here.
        bodyReached = true;
        timeoutCount++;
    }

    void scheduleTimerUnsafe(IoQueue& q)
    {
        // Raw `this` is the call target — no ownership.
        // weak_from_this() is the first bound method argument — does NOT
        // keep the object alive.
        q.post(std::bind_front(&TimerWorker::unsafeOnTimeout, this,
                               weak_from_this()));
    }
};

// ============================================================================
// Test 1 — Safe pattern: lambda captures weak_ptr; object destroyed first
// ============================================================================
//
// The handler in IoQueue holds only a weak_ptr.  The object is destroyed
// before drain() runs.  weak.lock() returns null → body is skipped safely.
//
TEST(WeakFromThisTimerPattern, SafeTimer_LambdaWeakPtr_ObjectDestroyedFirst)
{
    bool destroyed = false;
    auto obj = std::make_shared<TimerWorker>(destroyed);

    IoQueue ioQueue;
    obj->scheduleTimerSafe(ioQueue);

    // Destroy the object before the timer fires.
    // The lambda holds only a weak_ptr — no ownership — so destruction
    // proceeds immediately.
    obj.reset();
    EXPECT_TRUE(destroyed)
        << "Safe lambda holds no shared_ptr; object must be destroyed on reset";

    // drain() fires the timer handler.  weak.lock() returns null → body
    // is skipped.  No dangling pointer, no UB.
    ioQueue.drain();

    // bodyReached cannot be checked (object is gone), but the test must
    // not crash — that is the invariant.
}

// ============================================================================
// Test 2 — Unsafe pattern: raw `this` dangles after weak expires
// ============================================================================
//
// With bind_front(&Self::cb, this, weak_from_this()), `this` is the call
// target.  The weak_ptr bound as the first argument holds no ownership.
//
// When the object is destroyed before drain(), `this` becomes dangling.
// The handler still sits in IoQueue holding the dangling raw pointer.
// Any invocation would be UB.  This test verifies the invariant:
//
//   - The unsafe handler stores a raw `this` with no ownership.
//   - After obj.reset(), `this` is dangling.
//   - The only safe thing to do is NOT invoke the handler.
//
// We prove this by verifying the object is already destroyed after reset()
// (i.e. the handler's weak_ptr provided zero lifetime extension), which
// means any subsequent dispatch through raw `this` would be UB.
//
TEST(WeakFromThisTimerPattern, UnsafeTimer_RawPtrDanglesAfterWeakExpires)
{
    bool destroyed = false;
    auto obj = std::make_shared<TimerWorker>(destroyed);

    IoQueue ioQueue;
    obj->scheduleTimerUnsafe(ioQueue);

    // Drop the only external shared_ptr.
    // The handler in IoQueue binds weak_from_this() as an argument — this
    // does NOT keep the object alive (unlike shared_from_this()).
    // The object must be destroyed immediately.
    obj.reset();
    EXPECT_TRUE(destroyed)
        << "Unsafe pattern: weak_from_this() as bound arg provides no "
           "ownership; object must be destroyed when external shared_ptr drops";

    // At this point, the raw `this` stored as the call target inside the
    // queued handler is DANGLING.  Invoking it is undefined behavior.
    // Do NOT call ioQueue.drain() here — that would invoke UB.
    //
    // The safe pattern (Test 1) is immune: the lambda never stores a raw
    // pointer, so there is nothing dangling to invoke.
    //
    // Clear the queue without invoking to avoid UB in the test runner.
    ioQueue = IoQueue{};
}

// ============================================================================
// Test 3 — Safe pattern: object alive when timer fires → body runs correctly
// ============================================================================
//
// The object is kept alive by the external shared_ptr past drain().
// weak.lock() succeeds → onTimeout() body runs → timeoutCount incremented.
//
TEST(WeakFromThisTimerPattern, SafeTimer_CallbackInvokedWhenAlive)
{
    bool destroyed = false;
    auto obj = std::make_shared<TimerWorker>(destroyed);

    IoQueue ioQueue;
    obj->scheduleTimerSafe(ioQueue);

    EXPECT_FALSE(destroyed);
    EXPECT_FALSE(obj->bodyReached);

    // Object is still alive — weak.lock() will succeed.
    ioQueue.drain();

    EXPECT_TRUE(obj->bodyReached)
        << "Timer body must run when object is alive at dispatch time";
    EXPECT_EQ(obj->timeoutCount, 1);
    EXPECT_FALSE(destroyed)
        << "Object must still be alive; external shared_ptr still holds it";
}

} // anonymous namespace
