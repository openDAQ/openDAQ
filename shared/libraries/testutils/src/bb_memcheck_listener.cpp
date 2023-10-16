#include <testutils/bb_memcheck_listener.h>
#include <coretypes/intfs.h>

using namespace daq;

void DaqMemCheckListener::OnTestStart(const testing::TestInfo& info)
{
#ifndef NDEBUG
    daqClearTrackedObjects();
    objCount = daqGetTrackedObjectCount();
#endif
    MemCheckListener::OnTestStart(info);
}

void DaqMemCheckListener::OnTestEnd(const testing::TestInfo& info)
{
    daqClearErrorInfo();

    if (!info.result()->Failed())
    {
#ifndef NDEBUG
        size_t newObjCount = daqGetTrackedObjectCount();
        if (newObjCount != objCount)
        {
            if (!expectMemoryLeak)
            {
                daqPrintTrackedObjects();
                daqClearTrackedObjects();
                FAIL() << "Memory leaks detected (old obj count = " << objCount << ", new obj count = " << newObjCount << ")";
            }
            daqClearTrackedObjects();
        }
#endif
    }

    MemCheckListener::OnTestEnd(info);
}
