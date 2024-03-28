/*
 * Copyright 2022-2024 Blueberry d.o.o.
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

class MockDeviceModuleImpl : public daq::ImplementationOf<daq::IModule>
{
public:
    MockDeviceModuleImpl(daq::ContextPtr ctx);

    daq::ErrCode INTERFACE_FUNC getVersionInfo(daq::IVersionInfo** version) override;
    daq::ErrCode INTERFACE_FUNC getName(daq::IString** name) override;
    daq::ErrCode INTERFACE_FUNC getId(daq::IString** id) override;

    daq::ErrCode INTERFACE_FUNC getAvailableDevices(daq::IList** availableDevices) override;
    daq::ErrCode INTERFACE_FUNC getAvailableDeviceTypes(daq::IDict** deviceTypes) override;
    daq::ErrCode INTERFACE_FUNC acceptsConnectionParameters(daq::Bool* accepted, daq::IString* connectionString, daq::IPropertyObject* config) override;
    daq::ErrCode INTERFACE_FUNC createDevice(daq::IDevice** device, daq::IString* connectionString, daq::IComponent* parent, daq::IPropertyObject* config) override;

    daq::ErrCode INTERFACE_FUNC getAvailableFunctionBlockTypes(daq::IDict** functionBlockTypes) override;
    daq::ErrCode INTERFACE_FUNC createFunctionBlock(daq::IFunctionBlock** functionBlock, daq::IString* id, daq::IComponent* parent, daq::IString* localId, daq::IPropertyObject* config) override;

    daq::ErrCode INTERFACE_FUNC getAvailableServerTypes(daq::IDict** serverTypes) override;
    daq::ErrCode INTERFACE_FUNC createServer(daq::IServer** server, daq::IString* serverType, daq::IDevice* rootDevice, daq::IPropertyObject* config) override;

    daq::ErrCode INTERFACE_FUNC acceptsStreamingConnectionParameters(daq::Bool* accepted, daq::IString* connectionString, daq::IServerCapability* /*capability*/) override;
    daq::ErrCode INTERFACE_FUNC createStreaming(daq::IStreaming** streaming, daq::IString* connectionString, daq::IServerCapability* /*capability*/) override;

private:
    daq::ContextPtr ctx;
    size_t cnt = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(INTERNAL_FACTORY, MockDeviceModule, daq::IModule, daq::IContext*, ctx)
