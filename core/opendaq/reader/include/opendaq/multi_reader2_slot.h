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
#include <opendaq/component_ptr.h>
#include <opendaq/multi_reader2_domain.h>
#include <opendaq/multi_reader2_input_status.h>
#include <opendaq/multi_reader2_queue.h>
#include <opendaq/multi_reader2_sync.h>

#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>

BEGIN_NAMESPACE_OPENDAQ

namespace multi_reader2
{

/*!
 * @brief A late input's way back: Tracked from its failure until its ticks are within the limit of the main input's newest
 * again, then ClearQueued until the read that reaches the clear queued for it. A stalled main input takes the same way.
 */
class LateRecovery
{
public:
    enum class State
    {
        None,
        Tracked,
        ClearQueued
    };

    State state() const
    {
        return current;
    }
    // An ahead input is followed by the first sample of its newest packet, a behind one by its newest sample
    void start(bool ahead, std::optional<std::int64_t> tick)
    {
        current = State::Tracked;
        this->ahead = ahead;
        this->tick = tick;
    }
    void follow(const Relation& relation, std::int64_t ownFirst, std::optional<std::int64_t> newest)
    {
        if (current == State::Tracked)
            tick = ahead ? std::optional<std::int64_t>(relation.toMain(ownFirst)) : newest;
    }
    bool within(std::int64_t mainNewest, std::int64_t limit) const
    {
        return current == State::Tracked && tick && std::abs(*tick - mainNewest) <= limit;
    }
    void clearQueued()
    {
        current = State::ClearQueued;
    }
    void reset()
    {
        current = State::None;
    }

private:
    State current = State::None;
    bool ahead = false;
    std::optional<std::int64_t> tick;  // main tick compared with the main input's newest
};

/*!
 * @brief One input of the data manager: what it announced and has in force, its ticks against the main input, its queue and
 * its way back from a late failure. The methods change only this input; the rules that compare inputs are the data manager's.
 */
struct Slot
{
    // What applying an event changed on the input
    struct Applied
    {
        bool valueChanged = false;
        bool domainChanged = false;
        bool connectedNow = false;
    };

    // How a late input within the limit again comes back
    enum class Rejoin
    {
        Withdrawn,  // no read applied the failure yet: its marker is gone
        Queued,     // a clear is queued; the read that reaches it rejoins the input
        Folded      // the clear cancelled an event at the tail, so nothing is queued
    };

    ComponentPtr input;
    const void* key = nullptr;
    Queue queue;
    Announced latest;  // at the tail of the queue; what a reconfigure evaluates
    std::optional<TickDomain> latestTicks;
    std::optional<std::int64_t> nextOwn;  // input tick that continues the last ingested packet
    Announced current;                    // in force, moved only by read and configure
    std::optional<TickDomain> ticks;      // the domain in force, parsed
    std::optional<Relation> relation;     // to the main input
    bool accepted = true;                 // the owner's verdict
    MultiReader2InputError error = MultiReader2InputError::None;
    std::string errorText;
    SizeT sampleSize = 0;                    // in the read buffer, under the descriptor in force
    SyncCause syncFailed = SyncCause::None;  // survives an internal reconfigure that keeps the main input
    LateRecovery late;
    std::optional<std::int64_t> newestTick;  // main tick of the last ingested sample
    std::optional<std::int64_t> syncFirst;   // main tick of the first sample kept since synchronization started
    std::uint64_t descriptorStamp = 0;       // a new stamp reports the descriptor afresh
    bool restartSync = false;                // a read applied a gap or a domain change on it

    // Folds the event into the queue's tail and makes its descriptors the latest. Returns whether an event stays queued
    bool queueEvent(const Event& event);
    bool queueGap();
    // Puts the event in force: descriptors, ticks, SyncFailed and the way back. The relation is the data manager's
    Applied apply(const Event& event);
    // Queues a SyncFailed marker: behind the queued data when that data is aligned with the other inputs, else in front of it
    void fail(SyncCause cause, bool ahead, bool aligned);

    // The packet's relation to the main input and its newest sample in main ticks, kept whether or not the packet is queued
    std::optional<Relation> track(std::int64_t ownFirst, SizeT count, bool isMain, const TickDomain* main);
    // A packet of this many samples would pass 4 s of the input's data
    bool full(SizeT count) const;
    // Requires late.within
    Rejoin rejoin();
    // What the synchronization judges this input on; requires a relation
    SyncInput syncInput(std::int64_t mainFirst, std::int64_t mainDelta) const;

    // Requires a relation, which a contributing input always has
    std::int64_t mainTick(std::int64_t own) const;
};

}

END_NAMESPACE_OPENDAQ
