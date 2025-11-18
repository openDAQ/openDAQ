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
#include <testutils/memcheck_listener.h>
#include <gtest/gtest.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>
#include <coretypes/list_factory.h>

class DaqMemCheckListener : public MemCheckListener
{
protected:
    void OnTestStart(const testing::TestInfo& info) override
    {
#       ifndef NDEBUG
        daqClearTrackedObjects();
        objCount = daqGetTrackedObjectCount();
#       endif
        MemCheckListener::OnTestStart(info);
    }

    void OnTestEnd(const testing::TestInfo& info) override
    {
        daq::ListPtr<daq::IErrorInfo> errorInfoList = nullptr;
        daqGetErrorInfoList(&errorInfoList);
        if (errorInfoList.assigned() && errorInfoList.getCount() > 0)
        {
            ::testing::Message failMessage;
            failMessage << "Unresolved errors during the test: \n";
            for (const auto& errorInfo : errorInfoList)
            {
                daq::StringPtr message;
                errorInfo->getFormattedMessage(&message);
                if (message.assigned())
                    failMessage << message << "\n";
            }
            FAIL() << failMessage;
        }

        if (!info.result()->Failed())
        {
#           ifndef NDEBUG
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
#           endif
        }

        MemCheckListener::OnTestEnd(info);
    }

private:
#   ifndef NDEBUG
    size_t objCount = 0;
#   endif
};
