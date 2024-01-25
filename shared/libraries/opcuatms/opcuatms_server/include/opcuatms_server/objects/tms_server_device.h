/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <opendaq/device_ptr.h>
#include "opcuatms_server/objects/tms_server_function_block.h"
#include "opcuatms_server/objects/tms_server_channel.h"
#include "opcuatms_server/objects/tms_server_folder.h"
#include "opcuatms_server/objects/tms_server_component.h"
#include "opcuatms_server/objects/tms_server_property_object.h"
#include <list>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

class TmsServerDevice;
using TmsServerDevicePtr = std::shared_ptr<TmsServerDevice>;

class TmsServerDevice : public TmsServerComponent<DevicePtr>
{
public:
    using Super = TmsServerComponent<DevicePtr>;

    TmsServerDevice(const DevicePtr& object, const opcua::OpcUaServerPtr& server, const ContextPtr& context, const TmsServerContextPtr& tmsContext);

    bool createOptionalNode(const opcua::OpcUaNodeId& nodeId) override;
    
    void bindCallbacks() override;
    void addChildNodes() override;

    void createNonhierarchicalReferences() override;

protected:
    opcua::OpcUaNodeId getTmsTypeId() override;
    void populateDeviceInfo();
    void populateStreamingOptions();
    void addFbMethodNodes();
    void createAddFunctionBlockNode(const OpcUaNodeId& parentId);
    void createRemoveFunctionBlockNode(const OpcUaNodeId& parentId);
    void createGetAvailableFunctionBlockTypesNode(const OpcUaNodeId& parentId);
    void onGetAvailableFunctionBlockTypes(const NodeEventManager::MethodArgs& args);
    void onAddFunctionBlock(const NodeEventManager::MethodArgs& args);
    void onRemoveFunctionBlock(const NodeEventManager::MethodArgs& args);
    TmsServerFunctionBlockPtr addFunctionBlock(const StringPtr& fbTypeId, const OpcUaVariant& configVariant);
    TmsServerFunctionBlockPtr addFunctionBlock(const StringPtr& fbTypeId, const PropertyObjectPtr& config);
    void removeFunctionBlock(const StringPtr& localId);

    // TODO we need following list to keep this because of handlers. Move handlers (TmsServerObject) to context and use UA_NodeTypeLifecycle
    // for deleting it
    std::list<TmsServerSignalPtr> signals;
    std::list<TmsServerDevicePtr> devices;
    std::list<TmsServerFunctionBlockPtr> functionBlocks;
    std::list<TmsServerFolderPtr> folders;
    std::list<TmsServerComponentPtr> components;
    std::list<TmsServerPropertyObjectPtr> streamingOptions;
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
