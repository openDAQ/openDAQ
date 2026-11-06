#include <opendaq/multi_reader2_domain.h>
#include <opendaq/multi_reader2_queue.h>

BEGIN_NAMESPACE_OPENDAQ

namespace multi_reader2
{

void InputQueue::pushData(DataPacketPtr packet, SizeT count, std::optional<std::int64_t> firstTick, SizeT size)
{
    Entry entry;
    entry.packet = std::move(packet);
    entry.count = count;
    entry.firstTick = firstTick;
    entry.bytes = size;
    entries.pushBack(std::move(entry));

    samples += count;
    addBytes(size);
    if (events == 0)
        front += count;
}

void InputQueue::pushEvent(Event event)
{
    Entry entry;
    entry.event = std::move(event);
    entries.pushBack(std::move(entry));
    ++events;
}

void InputQueue::pushFrontEvent(Event event)
{
    Entry entry;
    entry.event = std::move(event);
    entries.pushFront(std::move(entry));
    ++events;
    front = 0;
}

bool InputQueue::cancelTrailingDisconnect()
{
    if (entries.empty() || !entries.back().event.has_value() || entries.back().event->kind != Event::Kind::Disconnected)
        return false;
    entries.popBack();
    --events;
    return true;
}

std::optional<std::int64_t> InputQueue::frontTick() const
{
    if (front == 0)
        return std::nullopt;
    return entries.front().firstTick;
}

Event InputQueue::popEvent()
{
    Event event = std::move(*entries.front().event);
    entries.popFront();
    --events;
    countFront();
    return event;
}

void InputQueue::dropFront()
{
    while (!entries.empty() && !entries.front().event.has_value())
        popData();
}

InputQueue::Backlog InputQueue::dropData()
{
    Backlog removed;
    Ring<Entry> kept;
    for (SizeT i = 0; i < entries.size(); ++i)
    {
        if (entries[i].event.has_value())
            kept.pushBack(std::move(entries[i]));
        else
            removed.pushBack(std::move(entries[i]));
    }
    entries = std::move(kept);
    front = 0;
    samples = 0;
    removeBytes(bytes);
    return removed;
}

void InputQueue::clear()
{
    entries.clear();
    front = 0;
    events = 0;
    samples = 0;
    removeBytes(bytes);
}

void InputQueue::addBytes(SizeT size)
{
    bytes += size;
    if (total)
        *total += size;
}

void InputQueue::removeBytes(SizeT size)
{
    bytes -= size;
    if (total)
        *total -= size;
}

// The run has one sample every step ticks, so the samples before the tick are counted, not walked
void InputQueue::discardBefore(std::int64_t tick)
{
    while (front > 0)
    {
        Entry& entry = entries.front();
        if (!entry.firstTick.has_value() || *entry.firstTick >= tick)
            return;

        const auto distance = checkedSub(tick, *entry.firstTick);
        if (distance.has_value())
        {
            // Rounded up without adding to a distance that may be close to the Int64 limit
            const auto behind = static_cast<SizeT>(*distance / step + (*distance % step != 0 ? 1 : 0));
            if (behind < entry.count)
            {
                advance(entry, behind);
                return;
            }
        }
        popData();
    }
}

// The data entry in front of the queue, with what is left of its samples and its bytes
void InputQueue::popData()
{
    const Entry& entry = entries.front();
    front -= entry.count;
    samples -= entry.count;
    removeBytes(entry.bytes);
    entries.popFront();
}

// Count samples of the front entry are read or discarded; the bytes stay until the packet leaves
void InputQueue::advance(Entry& entry, SizeT count)
{
    entry.first += count;
    entry.count -= count;
    // Only while samples are left: their ticks were checked at ingest, the tick after the last one was not
    if (entry.firstTick.has_value() && entry.count > 0)
        *entry.firstTick += static_cast<std::int64_t>(count) * step;
    front -= count;
    samples -= count;
}

void InputQueue::countFront()
{
    front = 0;
    for (SizeT i = 0; i < entries.size() && !entries[i].event.has_value(); ++i)
        front += entries[i].count;
}

}

END_NAMESPACE_OPENDAQ
