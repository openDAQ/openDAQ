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
 * Counts threads currently inside a small critical window (bracketed by a Scope), so a
 * slow-path thread can reclaim shared state safely: perform a seq_cst state transition
 * first (swap a pointer, set a closed flag, ...), then isIdle()/waitUntilIdle() proves no
 * window that observed the old state is still running. Everything is seq_cst, so a window
 * either sees the new state or is visible to the idleness check - never neither.
 */
class ActiveOperationTracker
{
public:
    struct Scope
    {
        explicit Scope(ActiveOperationTracker& tracker)
            : tracker(&tracker)
        {
            tracker.count.fetch_add(1, std::memory_order_seq_cst);
        }

        ~Scope()
        {
            tracker->count.fetch_sub(1, std::memory_order_release);
        }

        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
        Scope(Scope&&) = delete;
        Scope& operator=(Scope&&) = delete;

    private:
        ActiveOperationTracker* tracker;
    };

    [[nodiscard]] bool isIdle() const
    {
        return count.load(std::memory_order_seq_cst) == 0;
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
