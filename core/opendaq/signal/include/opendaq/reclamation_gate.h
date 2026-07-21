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
 * Reader counter for asymmetric (Dekker-style) reclamation protocols on the lock-free
 * data path. Fast-path threads bracket a tiny critical window with enter()/leave()
 * (via Guard); slow-path (config) threads first perform a seq_cst state transition
 * (publish a new pointer, set a closed flag, ...) and then wait for / check quiescence
 * before reclaiming whatever the old state referenced.
 *
 * Correctness argument, used by every gate in the data path: the guarded window's
 * enter() (A), its state read (B), the slow path's state transition (C) and its
 * quiescence check (D) are all seq_cst, hence part of the single seq_cst total order S.
 * If B observed the pre-transition state, B precedes C in S (a seq_cst read ordered
 * after C would have to observe the new state), therefore A precedes C as well, and
 * every D executed after C observes a non-quiescent gate until the window's leave() -
 * which is sequenced after whatever pin/copy the window performed. The slow path can
 * therefore never reclaim state between a fast-path read and its pin.
 *
 * The one-sided variant (quiescent() without waiting) is used by wait-free producers:
 * after their own seq_cst transition, quiescent()==true proves every reader of the old
 * state has already pinned it, so immediate reclamation is safe; otherwise the producer
 * defers reclamation to the slow path instead of waiting.
 */
class ReclamationGate
{
public:
    struct Guard
    {
        explicit Guard(ReclamationGate& gate)
            : gate(&gate)
        {
            gate.counter.fetch_add(1, std::memory_order_seq_cst);  // (A)
        }

        ~Guard()
        {
            gate->counter.fetch_sub(1, std::memory_order_release);
        }

        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;
        Guard(Guard&&) = delete;
        Guard& operator=(Guard&&) = delete;

    private:
        ReclamationGate* gate;
    };

    [[nodiscard]] bool quiescent() const
    {
        return counter.load(std::memory_order_seq_cst) == 0;       // (D)
    }

    void waitQuiescent() const
    {
        while (!quiescent())
            std::this_thread::yield();
    }

private:
    std::atomic<int> counter{0};
};

}

END_NAMESPACE_OPENDAQ
