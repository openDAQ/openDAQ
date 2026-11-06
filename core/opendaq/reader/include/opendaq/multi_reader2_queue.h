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

#include <algorithm>
#include <cstdint>
#include <optional>
#include <vector>

BEGIN_NAMESPACE_OPENDAQ

namespace multi_reader2
{

// Why an input is SyncFailed; the public error is the same, the cause picks the recovery rule and the message
enum class SyncCause
{
    None,
    Ahead,    // started more than the sync limit past the main input's samples; relative to the main input
    Stalled,  // sent nothing while the others sent more than the sync limit past it; the input's own
    OffGrid   // its samples fall between the main input's ticks; relative to the main input
};

// A change queued on one input, applied when a read reaches it
struct Event
{
    enum class Kind
    {
        Descriptor,    // value and domain: both descriptors after the event, so a removal is a null
        Connected,     // value and domain: the signal's descriptors
        Disconnected,
        Gap,           // the input's data does not continue its previous packet
        SyncFailed,    // a failure found at ingest, queued in front of the input's data
        Rejoined       // a stalled or ahead input is back within the recovery limit of the others
    };

    Kind kind;
    DataDescriptorPtr value;
    DataDescriptorPtr domain;
    SyncCause cause = SyncCause::None;
};

/*!
 * @brief A double-ended queue in one growing array, a power of two in size. The input queues hold one entry per packet,
 * and std::deque under MSVC holds one element per block for elements over 8 bytes, so it would allocate per packet. The
 * array halves again once it is less than a quarter full, so a backlog does not keep its memory.
 */
template <typename T>
class Ring
{
public:
    bool empty() const
    {
        return count == 0;
    }
    SizeT size() const
    {
        return count;
    }
    T& operator[](SizeT i)
    {
        return items[(head + i) & (items.size() - 1)];
    }
    const T& operator[](SizeT i) const
    {
        return items[(head + i) & (items.size() - 1)];
    }
    T& front()
    {
        return items[head];
    }
    const T& front() const
    {
        return items[head];
    }
    T& back()
    {
        return (*this)[count - 1];
    }
    const T& back() const
    {
        return (*this)[count - 1];
    }

    void pushBack(T item)
    {
        resize(count + 1);
        (*this)[count] = std::move(item);
        ++count;
    }
    void pushFront(T item)
    {
        resize(count + 1);
        head = (head + items.size() - 1) & (items.size() - 1);
        items[head] = std::move(item);
        ++count;
    }
    void popFront()
    {
        items[head] = T();
        head = (head + 1) & (items.size() - 1);
        --count;
        resize(count);
    }
    void popBack()
    {
        back() = T();
        --count;
        resize(count);
    }
    void clear()
    {
        items.clear();
        head = 0;
        count = 0;
    }

private:
    // Grows to fit needed entries, or shrinks once a quarter full; the entries keep their order from index 0
    void resize(SizeT needed)
    {
        SizeT capacity = items.size();
        if (needed > capacity)
            capacity = std::max<SizeT>(8, capacity * 2);
        else if (capacity > 64 && needed * 4 < capacity)
            capacity /= 2;
        else
            return;

        std::vector<T> moved(capacity);
        for (SizeT i = 0; i < count; ++i)
            moved[i] = std::move((*this)[i]);
        items = std::move(moved);
        head = 0;
    }

    std::vector<T> items;
    SizeT head = 0;
    SizeT count = 0;
};

/*!
 * @brief Internal to MultiReader2: one input's queue of data packets and events, in ingest order. Each data entry knows the
 * main tick of its first unread sample. The counts a read and the limits need are kept on every push and pop, so neither
 * ever walks the backlog; only taking an event off the front counts the run behind it.
 *
 * The front run is the data in front of the first event: what a read can deliver. A run holds one sample every step main
 * ticks, as a gap ends it with an event.
 */
class InputQueue
{
public:
    struct Entry
    {
        DataPacketPtr packet;
        SizeT first = 0;                        // first unread sample of the packet
        SizeT count = 0;                        // unread samples from first
        std::optional<std::int64_t> firstTick;  // main tick of sample first; unset when off the grid
        SizeT bytes = 0;                        // the packet's value and domain bytes, held until it leaves
        std::optional<Event> event;
    };
    // Entries taken off a queue, so the caller chooses where their packets are released
    using Backlog = Ring<Entry>;

    // The bytes of several queues together, kept with this queue's own; may be null
    SizeT* total = nullptr;

    // Main ticks from one sample to the next; set when the main input changes, with the queue empty
    void setStep(std::int64_t ticks)
    {
        step = ticks;
    }

    // Producer side
    void pushData(DataPacketPtr packet, SizeT count, std::optional<std::int64_t> firstTick, SizeT bytes);
    void pushEvent(Event event);
    void pushFrontEvent(Event event);
    // Removes a disconnect at the tail, with no data behind it; true when there was one
    bool cancelTrailingDisconnect();

    bool empty() const
    {
        return entries.empty();
    }
    bool frontIsEvent() const
    {
        return !entries.empty() && entries.front().event.has_value();
    }
    const Event& frontEvent() const
    {
        return *entries.front().event;
    }
    SizeT eventCount() const
    {
        return events;
    }
    // Samples in front of the first event
    SizeT frontCount() const
    {
        return front;
    }
    // Samples and packet bytes of all queued data
    SizeT sampleCount() const
    {
        return samples;
    }
    SizeT byteCount() const
    {
        return bytes;
    }
    bool hasData() const
    {
        return samples > 0;
    }
    // The main tick of the first sample in front; nullopt without data in front, or when that sample is off the grid
    std::optional<std::int64_t> frontTick() const;
    // The main tick of the last sample in front; requires frontTick()
    std::int64_t frontLastTick() const
    {
        return *frontTick() + static_cast<std::int64_t>(front - 1) * step;
    }

    // Consumer side. take hands the first count samples in front, which hold at least that many, to sink(packet, first,
    // count) one packet at a time, and removes them
    template <typename Sink>
    void take(SizeT count, Sink&& sink);
    void skip(SizeT count)
    {
        take(count, [](const DataPacketPtr&, SizeT, SizeT) {});
    }
    // Requires frontIsEvent()
    Event popEvent();

    // The data in front of the first event; all data, the events kept, returning what went; everything
    void dropFront();
    Backlog dropData();
    void clear();
    // The samples in front that lie before the main tick
    void discardBefore(std::int64_t tick);

private:
    void addBytes(SizeT size);
    void removeBytes(SizeT size);
    void popData();
    void advance(Entry& entry, SizeT count);
    void countFront();

    Ring<Entry> entries;
    std::int64_t step = 1;
    SizeT front = 0;
    SizeT events = 0;
    SizeT samples = 0;
    SizeT bytes = 0;
};

template <typename Sink>
void InputQueue::take(SizeT count, Sink&& sink)
{
    while (count > 0)
    {
        Entry& entry = entries.front();
        const SizeT taken = std::min(count, entry.count);
        sink(entry.packet, entry.first, taken);
        advance(entry, taken);
        count -= taken;
        if (entry.count == 0)
            popData();
    }
}

}

END_NAMESPACE_OPENDAQ
