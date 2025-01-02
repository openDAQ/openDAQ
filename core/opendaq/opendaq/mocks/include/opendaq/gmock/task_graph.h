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
#include <opendaq/task_graph.h>
#include <coretypes/intfs.h>
#include <gmock/gmock.h>
#include <coretypes/gmock/mock_ptr.h>

struct MockTaskGraph : daq::ImplementationOf<daq::ITaskGraph>
{
    typedef MockPtr<
        daq::ITaskGraph,
        daq::TaskGraphPtr,
        testing::StrictMock<MockTaskGraph>
    > Strict;

    MOCK_METHOD(daq::ErrCode, getName, (daq::IString** name), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, setName, (daq::IString* name), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, then, (daq::ITask* continuation), (override MOCK_CALL));
};
