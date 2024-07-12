/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <opendaq/scheduler.h>
#include <coretypes/intfs.h>
#include <opendaq/work.h>
#include <gmock/gmock.h>
#include <coretypes/gmock/mock_ptr.h>

struct MockScheduler : daq::ImplementationOf<daq::IScheduler>
{
    typedef MockPtr<
        daq::IScheduler,
        daq::SchedulerPtr,
        MockScheduler
    > Strict;

    MOCK_METHOD(daq::ErrCode, scheduleFunction, (daq::IFunction* work, daq::IAwaitable** awaitable), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, scheduleWork, (daq::IWork* work), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, scheduleGraph, (daq::ITaskGraph * graph, daq::IAwaitable** awaitable), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, stop, (), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, waitAll, (), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, isMultiThreaded, (daq::Bool* multiThreaded), (override MOCK_CALL));
};
