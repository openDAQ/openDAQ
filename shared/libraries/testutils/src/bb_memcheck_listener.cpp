#include <testutils/bb_memcheck_listener.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>
#include <coretypes/list_factory.h>

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
    daq::ListPtr<daq::IErrorInfo> errorInfoList = nullptr;
    daqGetErrorInfoList(&errorInfoList);
    if (errorInfoList.assigned() && errorInfoList.getCount() > 0)
    {
        ::testing::Message failMessage;
        failMessage << "Unresolved errors during the test: \n";
        for (const auto& errorInfo : errorInfoList)
        {
            StringPtr message;
            errorInfo->getFormatMessage(&message);
            if (message.assigned())
                failMessage << message << "\n";
        }
        FAIL() << failMessage;
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
