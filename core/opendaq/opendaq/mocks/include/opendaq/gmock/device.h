/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <opendaq/gmock/component.h>
#include <opendaq/device.h>

BEGIN_NAMESPACE_OPENDAQ

struct MockDevice : MockGenericSignalContainer<MockDevice, IDevice>
{
    typedef MockPtr<IDevice, typename daq::InterfaceToSmartPtr<IDevice>::SmartPtr, MockDevice> Strict;

    MOCK_METHOD(ErrCode, getInfo, (IDeviceInfo**), (override MOCK_CALL));
    MOCK_METHOD(ErrCode, getDomain, (IDeviceDomain**), (override MOCK_CALL));
    MOCK_METHOD(ErrCode, getInputsOutputsFolder, (IFolder**), (override MOCK_CALL));
    MOCK_METHOD(ErrCode, getCustomComponents, (IList**), (override MOCK_CALL));
    MOCK_METHOD(ErrCode, getSignals, (IList**, ISearchFilter*), (override MOCK_CALL));
    MOCK_METHOD(ErrCode, getSignalsRecursive, (IList**, ISearchFilter*), (override MOCK_CALL));
    MOCK_METHOD(ErrCode, getChannels, (IList**, ISearchFilter*), (override MOCK_CALL));
    MOCK_METHOD(ErrCode, getChannelsRecursive, (IList**, ISearchFilter*), (override MOCK_CALL));
    MOCK_METHOD(ErrCode, getDevices, (IList**, ISearchFilter*), (override MOCK_CALL));
    MOCK_METHOD(ErrCode, getAvailableDevices, (IList**), (override MOCK_CALL));
    MOCK_METHOD(ErrCode, getAvailableDeviceTypes, (IDict**), (override MOCK_CALL));
    MOCK_METHOD(ErrCode, addDevice, (IDevice**, IString*, IPropertyObject*), (override MOCK_CALL));
    MOCK_METHOD(ErrCode, removeDevice, (IDevice*), (override MOCK_CALL));
    MOCK_METHOD(ErrCode, getFunctionBlocks, (IList**, ISearchFilter*), (override MOCK_CALL));
    MOCK_METHOD(ErrCode, getAvailableFunctionBlockTypes, (IDict**), (override MOCK_CALL));
    MOCK_METHOD(ErrCode, addFunctionBlock, (IFunctionBlock**, IString*, IPropertyObject*), (override MOCK_CALL));
    MOCK_METHOD(ErrCode, removeFunctionBlock, (IFunctionBlock*), (override MOCK_CALL));
    MOCK_METHOD(ErrCode, saveConfiguration, (IString**), (override MOCK_CALL));
    MOCK_METHOD(ErrCode, loadConfiguration, (IString*), (override MOCK_CALL));
    MOCK_METHOD(ErrCode, getTicksSinceOrigin, (UInt*), (override MOCK_CALL));
    MOCK_METHOD(ErrCode, addStreaming, (IStreaming**, IString*, IPropertyObject*), (override MOCK_CALL));
    MOCK_METHOD(ErrCode, createDefaultAddDeviceConfig, (IPropertyObject**), (override MOCK_CALL));
};

END_NAMESPACE_OPENDAQ
