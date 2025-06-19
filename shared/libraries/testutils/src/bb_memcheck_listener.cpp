#include <testutils/bb_memcheck_listener.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>

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
    // daq::StringPtr errorMessage;
    // daqGetErrorInfoMessage(&errorMessage);
    daq::IList *errorInfoList = nullptr;
    daqGetErrorInfoList(&errorInfoList);
    if (errorInfoList != nullptr)
    {
        errorInfoList->releaseRef();
        // FAIL() << "Some errors was handled during the test: " << (errorMessage.assigned() ? errorMessage : "unknown error");
    }

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
