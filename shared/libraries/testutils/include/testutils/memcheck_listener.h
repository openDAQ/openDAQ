/*
 * Copyright 2022-2025 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#ifndef NDEBUG
    #ifdef _MSC_VER
        #define _CRTDBG_MAP_ALLOC  
        #include <crtdbg.h> 
    #endif // _MSC_VER 
#endif // !NDEBUG

#include <testutils/base_test_listener.h>

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

class MemCheckListener : public BaseTestListener
{
public:
    inline static bool expectMemoryLeak = false;

protected:
    void OnTestStart(const testing::TestInfo& info) override
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

    void OnTestEnd(const testing::TestInfo& info) override
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

private:
#ifndef NDEBUG
    #ifdef _MSC_VER
        _CrtMemState state1;
    #elif defined(__MINGW32__)
        size_t curBytesAllocated;
    #endif
#endif
};
