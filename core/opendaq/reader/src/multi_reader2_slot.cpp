#include <opendaq/multi_reader2_slot.h>

#include <cassert>
#include <cstdlib>

BEGIN_NAMESPACE_OPENDAQ

namespace multi_reader2
{

bool Slot::queueEvent(const Event& event)
{
    const bool queued = queue.pushEvent(event);
    if (!sameDescriptor(event.after.domain, latest.domain))
        latestTicks = parseDomain(event.after.domain);
    latest = event.after;
    return queued;
}

bool Slot::queueGap()
{
    Event gap{latest, latest};
    gap.gap = true;
    return queueEvent(gap);
}

Slot::Applied Slot::apply(const Event& event)
{
    Applied changed;
    changed.valueChanged = !sameDescriptor(event.after.value, current.value);
    changed.domainChanged = !sameDescriptor(event.after.domain, current.domain);
    changed.connectedNow = event.after.connected && !current.connected;
    current = event.after;
    if (changed.domainChanged)
        ticks = parseDomain(current.domain);

    if ((changed.domainChanged || changed.connectedNow || event.gap) && syncFailed == SyncCause::OffGrid)
        syncFailed = SyncCause::None;
    if (event.syncFailed != SyncCause::None)
        syncFailed = event.syncFailed;
    if (event.lateCleared && syncFailed == SyncCause::Late)
        syncFailed = SyncCause::None;
    if (syncFailed != SyncCause::Late)
        late = Late::None;
    return changed;
}

void Slot::fail(SyncCause cause, bool ahead, bool aligned)
{
    Event marker{current, current};
    marker.syncFailed = cause;
    if (cause == SyncCause::Late)
    {
        late = Late::Tracked;
        lateAhead = ahead;
        lateTick = ahead && queue.frontCount() > 0 ? mainTick(queue.newestPacketOwn()) : newestTick;
    }
    if (aligned)
        queue.pushEvent(marker);
    else
        queue.pushFrontEvent(marker);
}

std::optional<Relation> Slot::track(std::int64_t ownFirst, SizeT count, bool isMain, const TickDomain* main)
{
    const TickDomain& own = *latestTicks;
    std::optional<Relation> packetRelation;
    if (isMain)
        packetRelation = Relation{};
    else if (!queue.hasEvent() && relation)
        packetRelation = relation;
    else if (main)
        packetRelation = relate(own, *main);
    if (packetRelation)
        newestTick = packetRelation->toMain(ownFirst + static_cast<std::int64_t>(count - 1) * own.delta);
    return packetRelation;
}

bool Slot::full(SizeT bytes) const
{
    return queue.heldBytes() > 0 && queue.heldBytes() + bytes > queueLimitBytes;
}

// An ahead input is followed by its newest packet's first sample, a behind one by its newest sample
bool Slot::trackLate(const Relation& packetRelation, std::int64_t ownFirst)
{
    if (late == Late::Tracked)
        lateTick = lateAhead ? packetRelation.toMain(ownFirst) : *newestTick;
    return late == Late::Tracked;
}

bool Slot::lateWithin(std::int64_t mainNewest, std::int64_t limit) const
{
    return late == Late::Tracked && lateTick && std::abs(*lateTick - mainNewest) <= limit;
}

Slot::Rejoin Slot::rejoin()
{
    late = Late::None;
    // No read has applied the failure yet: withdrawn, the data behind it reads on as if it never failed
    if (syncFailed != SyncCause::Late && queue.withdrawSyncFailed())
        return Rejoin::Withdrawn;
    // Queued here, the clear lets the input rejoin with the data behind it once a read reaches it
    Event clear{latest, latest};
    clear.lateCleared = true;
    if (!queueEvent(clear))
        return Rejoin::Folded;
    late = Late::ClearQueued;
    return Rejoin::Queued;
}

SyncInput Slot::syncInput(std::int64_t mainFirst, std::int64_t mainDelta) const
{
    SyncInput in;
    in.empty = queue.empty();
    if (queue.frontCount() == 0)
        return in;
    std::int64_t remainder = 0;
    const std::int64_t first = relation->toMain(queue.firstOwn(ticks->delta), &remainder);
    in.first = first;
    in.last = mainTick(queue.lastOwn(ticks->delta));
    in.offGrid = remainder != 0 || (first - mainFirst) % mainDelta != 0;
    return in;
}

std::int64_t Slot::mainTick(std::int64_t own) const
{
    assert(relation);
    return relation->toMain(own);
}

}

END_NAMESPACE_OPENDAQ
