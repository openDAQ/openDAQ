#include <testutils/bb_memcheck_listener.h>
#include <coretypes/intfs.h>
#include <coretypes/errorinfo.h>
#include <coretypes/list_ptr.h>
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
    {
        ListObjectPtr<IList, IErrorInfo> errorInfoList;
        daqGetErrorInfoList(&errorInfoList);
        if (errorInfoList.assigned())
        {
            for (SizeT i = 0; i < errorInfoList.getCount(); i++)
            {
                const auto& errorInfo = errorInfoList[i];

                StringPtr message;
                errorInfo->getFormatMessage(&message);
                if (message.assigned())
                    GTEST_LOG_(ERROR) << i << "." << message;
            }
            if (errorInfoList.getCount())
                FAIL() << "Not all errors were handled";
        }
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
