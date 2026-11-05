#include <opendaq/multi_reader2_slot.h>

#include <cassert>

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

void Slot::queueGap()
{
    Event gap{latest, latest};
    gap.gap = true;
    queueEvent(gap);
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
    if (event.rejoined && (syncFailed == SyncCause::Stalled || syncFailed == SyncCause::Ahead))
        syncFailed = SyncCause::None;
    return changed;
}

void Slot::fail(SyncCause cause)
{
    Event marker{current, current};
    marker.syncFailed = cause;
    queue.pushFrontEvent(marker);
}

void Slot::track(std::int64_t ownFirst, SizeT count, bool isMain, const TickDomain* main)
{
    const TickDomain& own = *latestTicks;
    std::optional<Relation> packetRelation;
    if (isMain)
        packetRelation = Relation{};
    else if (!queue.hasEvent() && relation)
        packetRelation = relation;
    else if (main)
        packetRelation = relate(own, *main);
    if (!packetRelation)
        return;
    if (!firstTick)
        firstTick = packetRelation->toMain(ownFirst);
    newestTick = packetRelation->toMain(ownFirst + static_cast<std::int64_t>(count - 1) * own.delta);
}

bool Slot::over(std::int64_t seconds, SizeT bytes, SizeT addedCount, SizeT addedBytes) const
{
    if (queue.heldSamples() == 0)
        return false;
    const TickDomain& own = *latestTicks;
    return queue.heldBytes() + addedBytes > bytes ||
           static_cast<std::int64_t>(queue.heldSamples() + addedCount) * own.delta > secondsToTicks(own, seconds);
}

void Slot::rejoin()
{
    Event clear{latest, latest};
    clear.rejoined = true;
    queueEvent(clear);
}

SyncInput Slot::syncInput(std::int64_t mainFirst, std::int64_t mainDelta) const
{
    SyncInput in;
    if (queue.frontCount() == 0)
        return in;
    std::int64_t remainder = 0;
    const std::int64_t first = relation->toMain(queue.firstOwn(ticks->delta), &remainder);
    in.first = first;
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
