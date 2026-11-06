#include <testutils/benchmark_utils.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined(__linux__)
#include <sched.h>
#endif

namespace daq::testutils::benchmark
{

// Pins the whole process to one core class; the caller chooses which.
bool applyAffinity(uint64_t mask)
{
    if (mask == 0)
        return true;

#ifdef _WIN32
    return SetProcessAffinityMask(GetCurrentProcess(), static_cast<DWORD_PTR>(mask)) != 0;
#elif defined(__linux__)
    cpu_set_t set;
    CPU_ZERO(&set);
    for (unsigned cpu = 0; cpu < 64; ++cpu)
    {
        if (mask & (uint64_t(1) << cpu))
            CPU_SET(cpu, &set);
    }
    return sched_setaffinity(0, sizeof(set), &set) == 0;
#else
    return false;
#endif
}

}
