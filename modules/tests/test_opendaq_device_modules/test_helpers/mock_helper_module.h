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
#include <coretypes/intfs.h>
#include <opendaq/module.h>
#include <opendaq/context_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

namespace test_helpers
{

// helper module which advertises available devices info through a provided callback
class MockHelperModuleImpl : public ImplementationOf<IModule>
{
public:
    MockHelperModuleImpl(ContextPtr ctx, FunctionPtr availableDevicesCb);

    ErrCode INTERFACE_FUNC getVersionInfo(IVersionInfo** version) override;
    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC getId(IString** id) override;

    ErrCode INTERFACE_FUNC getAvailableDevices(IList** availableDevices) override;
    ErrCode INTERFACE_FUNC getAvailableDeviceTypes(IDict** deviceTypes) override;
    ErrCode INTERFACE_FUNC createDevice(IDevice** device, IString* connectionString, IComponent* parent, IPropertyObject* config) override;

    ErrCode INTERFACE_FUNC getAvailableFunctionBlockTypes(IDict** functionBlockTypes) override;
    ErrCode INTERFACE_FUNC createFunctionBlock(IFunctionBlock** functionBlock, IString* id, IComponent* parent, IString* localId, IPropertyObject* config) override;

    ErrCode INTERFACE_FUNC getAvailableServerTypes(IDict** serverTypes) override;
    ErrCode INTERFACE_FUNC createServer(IServer** server, IString* serverType, IDevice* rootDevice, IPropertyObject* config) override;

    ErrCode INTERFACE_FUNC createStreaming(IStreaming** streaming, IString* connectionString, IPropertyObject* /*config*/) override;

    ErrCode INTERFACE_FUNC createConnectionString(IString** connectionString, IServerCapability* serverCapability) override;
    ErrCode INTERFACE_FUNC getAvailableStreamingTypes(IDict** streamingTypes) override;

private:
    ContextPtr ctx;
    FunctionPtr availableDevicesCb;
};

}

END_NAMESPACE_OPENDAQ
