// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
//
// Unit tests for the bind_front / shared_from_this() lifetime invariant.
//
// Background
// ----------
// bmcweb async callbacks use std::bind_front to attach a member function
// to its owning object.  Two patterns appear with very different ownership
// semantics:
//
//   UNSAFE: bind_front(&Self::cb, this, shared_from_this())
//     `this`             — raw pointer, used as the implicit object.
//     shared_from_this() — extra bound argument that keeps the object alive
//       while the handler exists. But the raw pointer and its guardian
//       shared_ptr are two separate values with no enforced coupling.
//
//   SAFE: bind_front(&Self::cb, shared_from_this())
//     shared_from_this() — stored as the implicit object pointer inside the
//       callable.  The callable itself holds the reference; the object cannot
//       be destroyed while the handler is alive.
//
// The unsafe pattern breaks when the handler is destroyed without being
// invoked (e.g. the I/O queue is discarded): the bound shared_ptr may be
// the last owner, leaving any external raw `this` dangling.
//
// --- Test cases
//
//   BindFrontThisPattern — uses a minimal IoQueue mock to prove the full
//     async lifecycle: post, hold, drain-or-discard.
//
//     SafePattern_SharedPtrIsCallTarget
//       Handler keeps the object alive as the sole owner; object is
//       destroyed exactly when the handler is drained.
//
//     UnsafePattern_RawPtrOutlivesSharedPtr
//       Handler destroyed without being invoked (scope ends); the bound
//       shared_ptr was the last owner so the object is destroyed, leaving
//       any external raw `this` dangling.

#include <functional>
#include <memory>
#include <queue>
#include <utility>

#include <gtest/gtest.h>

namespace
{

// ============================================================================
// IoQueue — minimal model of a single-threaded Boost.Asio io_context queue.
//
// Replaces io_context in the BindFrontThisPattern suite so that tests can:
//   1. post() completion handlers exactly as async ops would enqueue them.
//   2. drain() them sequentially, mirroring single-threaded dispatch.
// ============================================================================
struct IoQueue
{
    void post(std::function<void()> handler)
    {
        queue.push(std::move(handler));
    }

    // Dispatch all queued handlers in FIFO order — mirrors io_context::run().
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
// AsyncWorker — subject for the BindFrontThisPattern suite.
//
// Models the bmcweb ConnectionInfo / websocket async pattern.
// ============================================================================
struct AsyncWorker : std::enable_shared_from_this<AsyncWorker>
{
    // Tracks whether the destructor has been called.
    bool& destroyed;
    int result = 0;

    explicit AsyncWorker(bool& destroyedFlag) : destroyed(destroyedFlag) {}
    AsyncWorker(const AsyncWorker&) = delete;
    AsyncWorker& operator=(const AsyncWorker&) = delete;
    AsyncWorker(AsyncWorker&&) = delete;
    AsyncWorker& operator=(AsyncWorker&&) = delete;
    ~AsyncWorker()
    {
        destroyed = true;
    }

    // Safe callback: shared_from_this() is the call target.
    // Paired with: bind_front(&AsyncWorker::safeCallback, shared_from_this())
    void safeCallback(int value)
    {
        result = value * 2;
    }

    // Unsafe callback: `this` is the raw call target; the leading shared_ptr
    // is the spurious lifetime-extension parameter added by the old pattern.
    // Paired with: bind_front(&AsyncWorker::unsafeCallback, this,
    //                          shared_from_this())
    void unsafeCallback(const std::shared_ptr<AsyncWorker>& /*self*/)
    {
        result = 1;
    }
};

// ============================================================================
// BindFrontThisPattern suite — IoQueue mock, full async lifecycle
// ============================================================================

// Test — Safe pattern: shared_ptr as call target
//
// bind_front stores shared_from_this() in the call-target slot.  The handler
// is post()ed to IoQueue, which becomes the sole shared_ptr owner after
// obj.reset().  drain() invokes the handler then destroys it, dropping
// the last ref and destroying the object.
TEST(BindFrontThisPattern, SafePattern_SharedPtrIsCallTarget)
{
    bool destroyed = false;
    auto obj = std::make_shared<AsyncWorker>(destroyed);

    IoQueue ioQueue;

    // shared_from_this() occupies the call-target slot of bind_front.
    ioQueue.post(std::bind_front(&AsyncWorker::safeCallback,
                                 obj->shared_from_this(), 21));

    // Drop the only external reference — IoQueue is now the SOLE owner.
    obj.reset();
    EXPECT_FALSE(destroyed)
        << "Object must still be alive; queued handler's shared_ptr holds it";

    // drain() invokes safeCallback(21) then destroys the handler.
    // shared_ptr refcount → 0 → object destroyed.
    ioQueue.drain();
    EXPECT_TRUE(destroyed)
        << "Object must be destroyed after the queued handler (sole owner) runs";
}

// Test — Unsafe pattern: raw `this` becomes dangling when handler is gone
//
// bind_front(&AsyncWorker::unsafeCallback, this, shared_from_this()) stores
// raw `this` as the call target and shared_from_this() as a bound argument.
// If the handler is destroyed without being drained (the IoQueue goes out of
// scope), the bound shared_ptr drops.  If that was the last owner the object
// is destroyed and the raw `this` copy held outside is now dangling.
TEST(BindFrontThisPattern, UnsafePattern_RawPtrOutlivesSharedPtr)
{
    bool destroyed = false;
    auto obj = std::make_shared<AsyncWorker>(destroyed);

    AsyncWorker* rawPtr = obj.get(); // non-owning observer — unsafe

    {
        IoQueue ioQueue;

        ioQueue.post(std::bind_front(&AsyncWorker::unsafeCallback, obj.get(),
                                     obj->shared_from_this()));

        // Drop the external shared_ptr — object alive only via queued handler.
        obj.reset();
        EXPECT_FALSE(destroyed)
            << "Object must survive: queued handler's bound shared_ptr holds it";

        // ioQueue goes out of scope — handler destroyed without being drained.
        // Its bound shared_ptr is the last ref → object destroyed.
    }
    EXPECT_TRUE(destroyed)
        << "Handler gone → bound shared_ptr dropped → object destroyed";

    // rawPtr now points to freed memory — do NOT dereference.
    EXPECT_TRUE(destroyed); // confirms rawPtr is dangling
    (void)rawPtr;
}

} // anonymous namespace
