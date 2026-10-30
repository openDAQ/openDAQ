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
#include <opendaq/multi_reader2_queue.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <vector>

BEGIN_NAMESPACE_OPENDAQ

namespace multi_reader2
{

// One input as the synchronization judges it: its queued samples in front of its first event, in main ticks
struct SyncInput
{
    std::optional<std::int64_t> first;  // unset without data in front
    std::optional<std::int64_t> last;
    bool empty = false;    // nothing queued at all
    bool offGrid = false;  // the first sample is off the main input's grid
};

/*!
 * @brief Judges the inputs in judged against the main input's queued samples, mainFirst to latest, and returns the common
 * start: the newest first sample among the main input and the inputs that pass. An input whose first sample is more than
 * limit past latest is late and ahead; one whose last sample is more than limit before the common start, or with nothing
 * queued while the main input has sent more than limit since synchronization started (sinceStart), is late and behind; one
 * off the grid fails as such. Pure: input(i) describes an input, fail(i, cause, ahead) is called once per failure, and the
 * inputs failed as ahead are removed from judged.
 */
template <class Input, class Fail>
std::int64_t judgeSync(std::int64_t mainFirst,
                       std::int64_t latest,
                       std::int64_t sinceStart,
                       std::int64_t limit,
                       std::vector<SizeT>& judged,
                       const Input& input,
                       const Fail& fail)
{
    std::int64_t common;
    for (;;)
    {
        common = mainFirst;
        for (SizeT i : judged)
            if (const auto first = input(i).first)
                common = std::max(common, *first);
        if (common - latest <= limit)
            break;
        // Inputs too far past the main input's latest sample proposed the common start: they are ahead
        for (auto it = judged.begin(); it != judged.end();)
        {
            const auto first = input(*it).first;
            if (first && *first - latest > limit)
            {
                fail(*it, SyncCause::Late, true);
                it = judged.erase(it);
            }
            else
                ++it;
        }
    }
    for (SizeT i : judged)
    {
        const SyncInput in = input(i);
        if (in.first)
        {
            if (common - *in.last > limit)
                fail(i, SyncCause::Late, false);
            else if (in.offGrid)
                fail(i, SyncCause::OffGrid, false);
        }
        else if (in.empty && sinceStart > limit)
            fail(i, SyncCause::Late, false);
    }
    return common;
}

}

END_NAMESPACE_OPENDAQ
