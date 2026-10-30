#include <opendaq/multi_reader2_queue.h>

#include <algorithm>
#include <cassert>

BEGIN_NAMESPACE_OPENDAQ

namespace multi_reader2
{

bool sameDescriptor(const DataDescriptorPtr& a, const DataDescriptorPtr& b)
{
    if (a.getObject() == b.getObject())
        return true;
    return a.assigned() && b.assigned() && a == b;
}

bool Announced::operator==(const Announced& other) const
{
    return connected == other.connected && sameDescriptor(value, other.value) && sameDescriptor(domain, other.domain);
}

void Queue::pushData(const DataPacketPtr& packet, std::int64_t ownFirst, SizeT count, SizeT bytes)
{
    entries.push_back({packet, ownFirst, count, 0, std::nullopt, bytes});
    held += bytes;
    if (events == 0)
    {
        front += count;
        ++frontEntries;
    }
}

bool Queue::pushEvent(const Event& event)
{
    if (!entries.empty() && entries.back().event)
    {
        Event& tail = *entries.back().event;
        tail.after = event.after;
        tail.gap = tail.gap || event.gap;
        tail.lateCleared = tail.lateCleared || event.lateCleared;
        if (event.syncFailed != SyncCause::None)
            tail.syncFailed = event.syncFailed;
        if (tail.changesSomething())
            return true;
        entries.pop_back();
        if (--events == 0)
            recount();
        return false;
    }
    if (!event.changesSomething())
        return false;
    entries.push_back({nullptr, 0, 0, 0, event});
    ++events;
    return true;
}

void Queue::pushFrontEvent(const Event& event)
{
    entries.push_front({nullptr, 0, 0, 0, event});
    ++events;
    front = 0;
    frontEntries = 0;
}

Event Queue::popEvent()
{
    Event event = *entries.front().event;
    entries.pop_front();
    --events;
    recount();
    return event;
}

bool Queue::withdrawSyncFailed()
{
    const auto it =
        std::find_if(entries.begin(), entries.end(), [](const Entry& e) { return e.event && e.event->syncFailed != SyncCause::None; });
    if (it == entries.end())
        return false;
    it->event->syncFailed = SyncCause::None;
    if (!it->event->changesSomething())
    {
        entries.erase(it);
        --events;
        recount();
    }
    return true;
}

std::int64_t Queue::firstOwn(std::int64_t delta) const
{
    const Entry& e = entries.front();
    return e.ownFirst + static_cast<std::int64_t>(e.consumed) * delta;
}

std::int64_t Queue::lastOwn(std::int64_t delta) const
{
    const Entry& e = entries[frontEntries - 1];
    return e.ownFirst + static_cast<std::int64_t>(e.count - 1) * delta;
}

std::int64_t Queue::newestPacketOwn() const
{
    return entries[frontEntries - 1].ownFirst;
}

void Queue::consume(SizeT n, const std::function<void(const DataPacketPtr&, SizeT, SizeT)>& sink)
{
    n = std::min(n, front);
    front -= n;
    while (n > 0)
    {
        Entry& e = entries.front();
        const SizeT k = std::min(n, e.count - e.consumed);
        if (sink)
            sink(e.packet, e.consumed, k);
        e.consumed += k;
        n -= k;
        if (e.consumed == e.count)
        {
            held -= e.bytes;
            entries.pop_front();
            --frontEntries;
        }
    }
}

// Debug builds check the counts kept on every push and pop against a walk of the queue
void Queue::checkCounts() const
{
#ifndef NDEBUG
    SizeT samples = 0;
    SizeT data = 0;
    for (const auto& e : entries)
    {
        if (e.event)
            break;
        samples += e.count - e.consumed;
        ++data;
    }
    SizeT bytes = 0;
    for (const auto& e : entries)
        bytes += e.bytes;
    assert(samples == front && data == frontEntries && bytes == held);
    assert(events == static_cast<SizeT>(std::count_if(entries.begin(), entries.end(), [](const Entry& e) { return e.event.has_value(); })));
#endif
}

void Queue::recount()
{
    front = 0;
    frontEntries = 0;
    for (const auto& e : entries)
    {
        if (e.event)
            break;
        front += e.count - e.consumed;
        ++frontEntries;
    }
}

}

END_NAMESPACE_OPENDAQ
