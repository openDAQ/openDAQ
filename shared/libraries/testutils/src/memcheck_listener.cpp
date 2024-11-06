#include <testutils/memcheck_listener.h>

bool MemCheckListener::expectMemoryLeak = false;

#ifndef NDEBUG
#ifdef _MSC_VER
/*
int AllocHook(
    int allocType, void* userData, size_t size, int blockType, long requestNumber, const unsigned char* filename, int lineNumber)
{
    return 1;
}
*/

#endif
#endif

void MemCheckListener::OnTestStart(const testing::TestInfo& info)
{
#ifndef NDEBUG
    #ifdef _MSC_VER
//        _CrtSetAllocHook(AllocHook);
        _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
        _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
        _CrtMemCheckpoint(&state1);
#elif defined(__MINGW32__)
        //curBytesAllocated = getBytesAllocated();
    #endif
#endif
    expectMemoryLeak = false;
    BaseTestListener::OnTestStart(info);
}

void MemCheckListener::OnTestEnd(const testing::TestInfo& info)
{
    if (info.result()->Passed())
    {
#ifndef NDEBUG
    #ifdef _MSC_VER
        _CrtMemState state2, state3;
        _CrtMemCheckpoint(&state2);

        int crtMemDifference = _CrtMemDifference(&state3, &state1, &state2);
        if (expectMemoryLeak)
        {
            if (!crtMemDifference)
            {
                FAIL() << "Memory leaks expected, but not detected";
            }
        }
        else if (crtMemDifference)
        {
//            _CrtMemDumpAllObjectsSince(&state1);
            FAIL() << "Memory leaks detected (" << state3.lTotalCount << " allocations)";
        }
    #elif defined(__MINGW32__)
        /*if (expectMemoryLeak)
              ASSERT_NE(getBytesAllocated(), curBytesAllocated) << "Memory leaks expected, but not detected";
          else
              ASSERT_EQ(getBytesAllocated(), curBytesAllocated) << "Memory leaks detected";
        */
    #endif
#endif
    }
}
