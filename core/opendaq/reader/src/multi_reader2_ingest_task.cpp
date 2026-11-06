#include <opendaq/multi_reader2_ingest_task.h>

BEGIN_NAMESPACE_OPENDAQ

namespace multi_reader2
{

// Scheduled and RunningDirty already promise a drain that starts after this packet was queued, so nothing is scheduled
bool IngestTask::notify()
{
    int s = state.load();
    for (;;)
    {
        if (s == Idle && state.compare_exchange_weak(s, Scheduled))
            return true;
        if (s == Running && state.compare_exchange_weak(s, RunningDirty))
            return false;
        if (s == Scheduled || s == RunningDirty)
            return false;
    }
}

// Running is set before the drain, so a packet queued after the drain passed its port turns it RunningDirty. One drain per
// run: a reader that never goes idle hands its worker back between runs
bool IngestTask::run(const std::function<void()>& drain)
{
    state.store(Running);
    try
    {
        drain();
    }
    catch (...)
    {
        // Idle, so the next notification schedules again; ports the drain did not reach stay marked
        state.store(Idle);
        throw;
    }
    int expected = Running;
    if (state.compare_exchange_strong(expected, Idle))
        return false;
    state.store(Scheduled);
    return true;
}

void IngestTask::cancel()
{
    state.store(Idle);
}

bool IngestTask::idle() const
{
    return state.load() == Idle;
}
}

END_NAMESPACE_OPENDAQ
