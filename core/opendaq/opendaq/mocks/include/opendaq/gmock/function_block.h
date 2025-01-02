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
#include <opendaq/function_block_ptr.h>
#include <opendaq/channel_ptr.h>
#include <coretypes/intfs.h>
#include <gmock/gmock.h>
#include <coretypes/gmock/mock_ptr.h>
#include <opendaq/gmock/component.h>

BEGIN_NAMESPACE_OPENDAQ

template <class Class, class TInterface>
struct MockGenericFunctionBlock : MockGenericSignalContainer<Class, TInterface>
{
    MOCK_METHOD(daq::ErrCode, getFunctionBlockType, (daq::IFunctionBlockType** type), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getInputPorts, (daq::IList** port, daq::ISearchFilter* searchFilter), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getSignals, (daq::IList** signals, daq::ISearchFilter* searchFilter), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getSignalsRecursive, (daq::IList** signal, daq::ISearchFilter* searchFilter), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getStatusSignal, (daq::ISignal** signal), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getFunctionBlocks, (daq::IList** functionBlocks, daq::ISearchFilter* searchFilter), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, getAvailableFunctionBlockTypes, (daq::IDict** functionBlockTypes), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, addFunctionBlock, (daq::IFunctionBlock** functionBlock, daq::IString* typeId, daq::IPropertyObject* config), (override MOCK_CALL));
    MOCK_METHOD(daq::ErrCode, removeFunctionBlock, (daq::IFunctionBlock* functionBlock), (override MOCK_CALL));

    MockGenericFunctionBlock()
        : MockGenericSignalContainer<Class, TInterface>()
    {
    }
};

struct MockFunctionBlock: MockGenericFunctionBlock<MockFunctionBlock, IFunctionBlock>
{
};

struct MockChannel : MockGenericFunctionBlock<MockChannel, IChannel>
{
};

END_NAMESPACE_OPENDAQ
