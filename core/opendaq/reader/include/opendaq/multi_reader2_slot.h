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
#include <optional>
#include <string>

BEGIN_NAMESPACE_OPENDAQ

namespace multi_reader2
{

/*!
 * @brief One input of the data manager: what it announced and has in force, its ticks against the main input, its queue and
 * its way back from a late failure. The methods change only this input; the rules that compare inputs are the data manager's.
 */
struct Slot
{
    // The queue's memory bound: far above transport skew between ports, a hard limit when the main input stops or the consumer lags
    static constexpr SizeT queueLimitBytes = SizeT(32) << 20;

    // What applying an event changed on the input
    struct Applied
    {
        bool valueChanged = false;
        bool domainChanged = false;
        bool connectedNow = false;
    };

    // A late input's way back: Tracked from fail until its ticks are within the limit again, ClearQueued until a read rejoins it
    enum class Late
    {
        None,
        Tracked,
        ClearQueued
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
    bool capped = false;                  // packets past the memory limit were dropped: the next kept one is a gap
    Announced current;                    // in force, moved only by read and configure
    std::optional<TickDomain> ticks;      // the domain in force, parsed
    std::optional<Relation> relation;     // to the main input
    bool accepted = true;                 // the owner's verdict
    MultiReader2InputError error = MultiReader2InputError::None;
    std::string errorText;
    SizeT sampleSize = 0;                    // in the read buffer, under the descriptor in force
    SyncCause syncFailed = SyncCause::None;  // survives an internal reconfigure that keeps the main input
    Late late = Late::None;
    bool lateAhead = false;                  // compared by the first sample of its newest packet, else by its newest sample
    std::optional<std::int64_t> lateTick;    // main tick compared with the main input's newest
    std::optional<std::int64_t> newestTick;  // main tick of the last ingested sample
    bool descriptorChanged = false;          // pending for the next status
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
    // A packet of this many bytes would pass the memory limit
    bool full(SizeT bytes) const;
    // Moves a tracked late input's tick with a packet it sends; returns whether it is tracked
    bool trackLate(const Relation& packetRelation, std::int64_t ownFirst);
    bool lateWithin(std::int64_t mainNewest, std::int64_t limit) const;
    // Requires lateWithin
    Rejoin rejoin();
    // What the synchronization judges this input on; requires a relation
    SyncInput syncInput(std::int64_t mainFirst, std::int64_t mainDelta) const;

    // Requires a relation, which a contributing input always has
    std::int64_t mainTick(std::int64_t own) const;
};

}

END_NAMESPACE_OPENDAQ
