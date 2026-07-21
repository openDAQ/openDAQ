/*
 * Copyright 2022-2026 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <coretypes/common.h>

#include <atomic>
#include <thread>

BEGIN_NAMESPACE_OPENDAQ

namespace details
{

/*
 * Counts threads currently inside a small critical window, so that another thread can
 * tell when it is safe to reclaim (free or reuse) memory those windows might still be
 * touching. Fast-path threads bracket their window with a Scope; slow-path (config)
 * threads first perform a seq_cst state transition (publish a new pointer, set a closed
 * flag, ...) and then check isIdle() / call waitUntilIdle() before reclaiming whatever
 * the old state referenced.
 *
 * Correctness argument, shared by every use on the data path: the window's counter
 * increment (A), its state read (B), the slow path's state transition (C) and its
 * idleness check (D) are all seq_cst, hence part of the single seq_cst total order S.
 * If B observed the pre-transition state, B precedes C in S (a seq_cst read ordered
 * after C would have to observe the new state), therefore A precedes C as well, and
 * every D executed after C observes a non-idle counter until the window ends - which is
 * sequenced after whatever pin/copy the window performed. The slow path can therefore
 * never reclaim state between a fast-path read and its pin.
 *
 * The one-sided variant (isIdle() without waiting) is used by wait-free producers:
 * after their own seq_cst transition, isIdle()==true proves every user of the old
 * state has already pinned it, so immediate reclamation is safe; otherwise the producer
 * defers reclamation to the slow path instead of waiting.
 */
class ActivityCounter
{
public:
    struct Scope
    {
        explicit Scope(ActivityCounter& counter)
            : counter(&counter)
        {
            counter.count.fetch_add(1, std::memory_order_seq_cst);  // (A)
        }

        ~Scope()
        {
            counter->count.fetch_sub(1, std::memory_order_release);
        }

        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
        Scope(Scope&&) = delete;
        Scope& operator=(Scope&&) = delete;

    private:
        ActivityCounter* counter;
    };

    [[nodiscard]] bool isIdle() const
    {
        return count.load(std::memory_order_seq_cst) == 0;          // (D)
    }

    void waitUntilIdle() const
    {
        while (!isIdle())
            std::this_thread::yield();
    }

private:
    std::atomic<int> count{0};
};

}

END_NAMESPACE_OPENDAQ
