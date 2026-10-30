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
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/data_packet_ptr.h>

#include <cstdint>
#include <deque>
#include <functional>
#include <optional>

BEGIN_NAMESPACE_OPENDAQ

namespace multi_reader2
{

// Why an input is SyncFailed; each cause recovers by its own rule
enum class SyncCause
{
    None,
    Late,
    OffGrid
};

// Equal by value, or both null; the identity check first keeps the common case cheap
bool sameDescriptor(const DataDescriptorPtr& a, const DataDescriptorPtr& b);

// What the signal on an input announced: its connection and its descriptors
struct Announced
{
    bool connected = false;
    DataDescriptorPtr value;
    DataDescriptorPtr domain;

    bool operator==(const Announced& other) const;
};

// A queued event: the net effect of consecutive events with no data between them
struct Event
{
    Announced before;  // in force in front of the event
    Announced after;
    bool gap = false;                        // the input's continuity broke
    SyncCause syncFailed = SyncCause::None;  // a failure found at ingest
    bool lateCleared = false;                // a late input came back within the limit here

    bool changesSomething() const
    {
        return !(before == after) || gap || syncFailed != SyncCause::None || lateCleared;
    }
};

/*!
 * @brief One input's queue: data packets and events in ingest order. The samples in front of the first event are counted on
 * every push and pop, so a read never walks the backlog.
 */
class Queue
{
public:
    // A data packet whose first sample is the input's tick ownFirst, holding bytes of memory until it is consumed
    void pushData(const DataPacketPtr& packet, std::int64_t ownFirst, SizeT count, SizeT bytes = 0);

    // Folds into a tail event, removing it when nothing changes so the data joins up; returns whether an event stays at the tail
    bool pushEvent(const Event& event);

    // A failure marker in front of everything queued
    void pushFrontEvent(const Event& event);

    bool empty() const
    {
        return entries.empty();
    }
    bool hasEvent() const
    {
        return events > 0;
    }
    bool frontIsEvent() const
    {
        return !entries.empty() && entries.front().event.has_value();
    }
    SizeT heldBytes() const
    {
        return held;
    }
    SizeT frontCount() const
    {
        checkCounts();
        return front;
    }

    // Requires frontIsEvent()
    Event popEvent();
    // Clears the queued failure marker; one that then changes nothing is removed. Returns whether one was queued
    bool withdrawSyncFailed();

    // Input ticks of the first and last sample in front of the first event; require frontCount() > 0
    std::int64_t firstOwn(std::int64_t delta) const;
    std::int64_t lastOwn(std::int64_t delta) const;
    // Input tick of the first sample of the newest data packet in front of the first event
    std::int64_t newestPacketOwn() const;

    // Hands the next n samples in front of the first event to sink, packet by packet, and removes them
    void consume(SizeT n, const std::function<void(const DataPacketPtr&, SizeT first, SizeT count)>& sink);
    void drop(SizeT n)
    {
        consume(n, nullptr);
    }

private:
    struct Entry
    {
        DataPacketPtr packet;
        std::int64_t ownFirst = 0;
        SizeT count = 0;
        SizeT consumed = 0;
        std::optional<Event> event;
        SizeT bytes = 0;
    };

    void recount();
    void checkCounts() const;

    std::deque<Entry> entries;
    SizeT front = 0;         // samples in front of the first event
    SizeT frontEntries = 0;  // data entries in front of the first event
    SizeT events = 0;
    SizeT held = 0;  // bytes of the queued packets
};

}

END_NAMESPACE_OPENDAQ
