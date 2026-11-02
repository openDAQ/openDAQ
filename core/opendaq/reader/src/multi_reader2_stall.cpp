#include <opendaq/custom_log.h>
#include <opendaq/multi_reader2_stall.h>

#include <algorithm>
#include <cstdlib>

BEGIN_NAMESPACE_OPENDAQ

namespace multi_reader2
{

namespace
{

bool contributing(const Slot& s, bool valid)
{
    return valid && s.error == MultiReader2InputError::None;
}

// Main ticks the others sent past the input's newest sample; nullopt for an input with something queued
std::optional<std::int64_t> silence(const std::vector<Slot>& slots, const Slot& s, bool valid, const TickDomain& main)
{
    if (!stallEnabled || !contributing(s, valid) || !s.queue.empty())
        return std::nullopt;
    std::optional<std::int64_t> newest;
    std::optional<std::int64_t> first;
    for (const auto& other : slots)
    {
        if (&other == &s || !contributing(other, valid) || !other.newestTick)
            continue;
        newest = std::max(newest.value_or(*other.newestTick), *other.newestTick);
        first = std::min(first.value_or(*other.firstTick), *other.firstTick);
    }
    if (!newest)
        return std::nullopt;
    return *newest - s.newestTick.value_or(*first - main.delta);
}

}

std::string stalledText()
{
    return fmt::format("not synchronized, no data for {} s", queueLimitSeconds);
}

bool stallPending(const std::vector<Slot>& slots, bool valid)
{
    return stallEnabled &&
           std::any_of(slots.begin(), slots.end(), [&](const Slot& s) { return contributing(s, valid) && s.queue.empty(); });
}

bool stallDue(const std::vector<Slot>& slots, bool valid, const TickDomain& main)
{
    const std::int64_t limit = secondsToTicks(main, queueLimitSeconds);
    return std::any_of(slots.begin(),
                       slots.end(),
                       [&](const Slot& s)
                       {
                           const auto ticks = silence(slots, s, valid, main);
                           return ticks && *ticks > limit;
                       });
}

void stall(std::vector<Slot>& slots, bool valid, const TickDomain& main, const LoggerComponentPtr& loggerComponent)
{
    const std::int64_t limit = secondsToTicks(main, queueLimitSeconds);
    std::vector<std::int64_t> silent(slots.size(), 0);
    for (SizeT i = 0; i < slots.size(); ++i)
        silent[i] = silence(slots, slots[i], valid, main).value_or(0);
    for (SizeT i = 0; i < slots.size(); ++i)
    {
        if (silent[i] <= limit)
            continue;
        slots[i].syncFailed = SyncCause::Stalled;
        if (loggerComponent.assigned())
        {
            LOG_T("{}: stalled, the other inputs sent {} main ticks past its newest sample, limit {}",
                  slots[i].input.getLocalId().toStdString(),
                  silent[i],
                  limit);
        }
    }
}

void checkRejoin(std::vector<Slot>& slots, SizeT i, const TickDomain& main, const LoggerComponentPtr& loggerComponent)
{
    Slot& s = slots[i];
    if (!stallEnabled || (s.syncFailed != SyncCause::Stalled && s.syncFailed != SyncCause::Ahead) || s.queue.hasEvent() || !s.newestTick)
        return;
    std::optional<std::int64_t> newest;
    for (const auto& other : slots)
        if (&other != &s && other.error == MultiReader2InputError::None && other.newestTick)
            newest = std::max(newest.value_or(*other.newestTick), *other.newestTick);
    if (!newest || std::abs(*newest - *s.newestTick) > secondsToTicks(main, rejoinSeconds))
        return;
    s.rejoin();
    if (loggerComponent.assigned())
    {
        LOG_T("{}: rejoins, {} main ticks from the newest sample of the other inputs",
              s.input.getLocalId().toStdString(),
              *newest - *s.newestTick);
    }
}

}

END_NAMESPACE_OPENDAQ
