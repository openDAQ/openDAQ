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

// Over the contributing inputs that sent a sample: the newest tick, the one input that holds it, the newest of the others,
// and the earliest first tick. One pass, so an input's silence needs no pass over the others
struct Senders
{
    std::optional<std::int64_t> newest;
    std::optional<std::int64_t> second;
    std::optional<std::int64_t> first;
    const Slot* newestSlot = nullptr;
};

Senders senders(const std::vector<Slot>& slots, bool valid)
{
    Senders p;
    for (const auto& s : slots)
    {
        if (!contributing(s, valid) || !s.newestTick)
            continue;
        if (!p.newest || *s.newestTick > *p.newest)
        {
            p.second = p.newest;
            p.newest = s.newestTick;
            p.newestSlot = &s;
        }
        else if (!p.second || *s.newestTick > *p.second)
            p.second = s.newestTick;
        p.first = std::min(p.first.value_or(*s.firstTick), *s.firstTick);
    }
    return p;
}

// Main ticks the others sent past the input's newest sample; nullopt for an input with something queued. An input that never
// sent is not among the senders, so their earliest first tick is the others'
std::optional<std::int64_t> silence(const Senders& p, const Slot& s, bool valid, const TickDomain& main)
{
    if (!stallEnabled || !contributing(s, valid) || !s.queue.empty())
        return std::nullopt;
    const auto newest = &s == p.newestSlot ? p.second : p.newest;
    if (!newest)
        return std::nullopt;
    return *newest - (s.newestTick ? *s.newestTick : *p.first - main.delta);
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
    if (!stallPending(slots, valid))
        return false;
    const std::int64_t limit = secondsToTicks(main, queueLimitSeconds);
    const Senders p = senders(slots, valid);
    return std::any_of(slots.begin(),
                       slots.end(),
                       [&](const Slot& s)
                       {
                           const auto ticks = silence(p, s, valid, main);
                           return ticks && *ticks > limit;
                       });
}

void stall(std::vector<Slot>& slots, bool valid, const TickDomain& main, const LoggerComponentPtr& loggerComponent)
{
    const std::int64_t limit = secondsToTicks(main, queueLimitSeconds);
    const Senders p = senders(slots, valid);
    std::vector<std::int64_t> silent(slots.size(), 0);
    for (SizeT i = 0; i < slots.size(); ++i)
        silent[i] = silence(p, slots[i], valid, main).value_or(0);
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
    // With every other input in error there is nothing to compare with, and the input's own packet rejoins it
    if (newest && std::abs(*newest - *s.newestTick) > secondsToTicks(main, rejoinSeconds))
        return;
    s.rejoin();
    if (loggerComponent.assigned())
    {
        LOG_T("{}: rejoins, {} main ticks from the newest sample of the other inputs",
              s.input.getLocalId().toStdString(),
              newest ? *newest - *s.newestTick : 0);
    }
}

}

END_NAMESPACE_OPENDAQ
