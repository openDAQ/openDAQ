#include <opendaq/custom_log.h>
#include <opendaq/multi_reader2_stall.h>

BEGIN_NAMESPACE_OPENDAQ

namespace multi_reader2
{

bool checkStalled(Slot& main, const Slot& other, bool streaming, std::int64_t syncLimit, const LoggerComponentPtr& loggerComponent)
{
    if (other.queue.hasEvent() || main.queue.hasEvent() || !other.newestTick)
        return false;
    std::optional<std::int64_t> from;
    if (streaming)
        from = main.newestTick;
    else if (main.queue.frontCount() == 0)
        from = other.syncFirst;
    // As the queue limit: beyond it the other inputs lose data anyway
    const std::int64_t limit = queueLimitSyncLimits * syncLimit;
    if (!from || *other.newestTick - *from <= limit)
        return false;
    main.fail(SyncCause::Late, false, streaming);
    if (loggerComponent.assigned())
    {
        LOG_T("{}: main input stalled, {} is {} main ticks past it, limit {}",
              main.input.getLocalId().toStdString(),
              other.input.getLocalId().toStdString(),
              *other.newestTick - *from,
              limit);
    }
    return true;
}

}

END_NAMESPACE_OPENDAQ
