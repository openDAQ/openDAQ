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

#include <opendaq/instance_factory.h>
#include <opcuaclient/browse_request.h>
#include <opcuaclient/browser/opcuabrowser.h>
#include <open62541/di_nodeids.h>
#include <open62541/daqdevice_nodeids.h>
#include "opendaq/mock/mock_device_module.h"
#include "opendaq/mock/mock_fb_module.h"
#include "opendaq/mock/mock_physical_device.h"
#include "open62541/daqbsp_nodeids.h"
#include <coreobjects/authentication_provider_factory.h>

namespace test_helpers
{
    inline daq::InstancePtr SetupInstance()
    {
        using namespace daq;
        const auto logger = Logger();
        const auto moduleManager = ModuleManager("[[none]]");
        const auto authenticationProvider = AuthenticationProvider();
        const auto context = Context(nullptr, logger, TypeManager(), moduleManager, authenticationProvider);

        const ModulePtr deviceModule(MockDeviceModule_Create(context));
        moduleManager.addModule(deviceModule);

        const ModulePtr fbModule(MockFunctionBlockModule_Create(context));
        moduleManager.addModule(fbModule);

        auto instance = InstanceCustom(context, "localInstance");
        instance.addDevice("daqmock://client_device");
        instance.addDevice("daqmock://phys_device");
        instance.addFunctionBlock("mock_fb_uid");

        return instance;
    }

    inline daq::opcua::OpcUaNodeId BrowseForChild(const daq::opcua::OpcUaClientPtr& client,
                                                            const daq::opcua::OpcUaNodeId& rootNodeId,
                                                            const std::string& browseName)
    {
        using namespace daq::opcua;
        std::vector<OpcUaNodeId> results;

        BrowseRequest request(rootNodeId, OpcUaNodeClass::Object);
        OpcUaBrowser browser(request, client);
        auto browseResult = browser.browse();

        for (const UA_ReferenceDescription& reference : browseResult)
        {
            if (daq::opcua::utils::ToStdString(reference.browseName.name) == browseName)
                return OpcUaNodeId(reference.nodeId.nodeId);
        }

        return {};
    }

    inline std::vector<daq::opcua::OpcUaNodeId> BrowseForChildWithTypeId(const daq::opcua::OpcUaClientPtr& client,
                                                                                   const daq::opcua::OpcUaNodeId& rootNodeId,
                                                                                   const daq::opcua::OpcUaNodeId& typeNodeId)
    {
        using namespace daq::opcua;
        std::vector<OpcUaNodeId> results;

        BrowseRequest request(rootNodeId, OpcUaNodeClass::Object);
        OpcUaBrowser browser(request, client);
        auto browseResult = browser.browse();

        for (const UA_ReferenceDescription& reference : browseResult)
        {
            if (OpcUaNodeId(reference.typeDefinition.nodeId) == typeNodeId)
                results.push_back(OpcUaNodeId(reference.nodeId.nodeId));
        }

        return results;
    }

    inline std::vector<daq::opcua::OpcUaNodeId> BrowseSubDevices(const daq::opcua::OpcUaClientPtr& client,
                                                                           const daq::opcua::OpcUaNodeId& nodeId)
    {
        using namespace daq::opcua;
        return BrowseForChildWithTypeId(client, nodeId, OpcUaNodeId(NAMESPACE_DAQDEVICE, UA_DAQDEVICEID_DAQDEVICETYPE));
    }

    inline std::vector<daq::opcua::OpcUaNodeId> BrowseFunctionBlocks(const daq::opcua::OpcUaClientPtr& client,
                                                                               const daq::opcua::OpcUaNodeId& nodeId)
    {
        using namespace daq::opcua;
        return BrowseForChildWithTypeId(client, nodeId, OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_FUNCTIONBLOCKTYPE));
    }

    inline std::vector<daq::opcua::OpcUaNodeId> BrowseChannels(const daq::opcua::OpcUaClientPtr& client,
                                                                         const daq::opcua::OpcUaNodeId& nodeId)
    {
        using namespace daq::opcua;
        return BrowseForChildWithTypeId(client, nodeId, OpcUaNodeId(NAMESPACE_DAQDEVICE, UA_DAQDEVICEID_CHANNELTYPE));
    }

    inline std::vector<daq::opcua::OpcUaNodeId> BrowseSignals(const daq::opcua::OpcUaClientPtr& client,
                                                                        const daq::opcua::OpcUaNodeId& nodeId)
    {
        using namespace daq::opcua;
        return BrowseForChildWithTypeId(client, nodeId, OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_SIGNALTYPE));
    }

    inline daq::opcua::OpcUaNodeId GetMockPhysicalDevice(const daq::opcua::OpcUaClientPtr& client)
    {
        using namespace daq::opcua;
        auto firstLvlDevices = BrowseSubDevices(client, OpcUaNodeId(NAMESPACE_DI, UA_DIID_DEVICESET));
        auto secondLvlDevices = BrowseSubDevices(client, firstLvlDevices[0]);
        for (const auto& dev : secondLvlDevices)
        {
            if (client->readBrowseName(dev) == "mockdev")
                return dev;
        }
        throw std::runtime_error("Mock device not found");
    }
}
