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
#include <opendaq/function_block_ptr.h>
#include "opcuatms_server/objects/tms_server_property_object.h"
#include "opcuatms_server/objects/tms_server_signal.h"
#include "opcuatms_server/objects/tms_server_input_port.h"
#include <list>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

template <typename T = FunctionBlockPtr>
class TmsServerFunctionBlock;

using TmsServerFunctionBlockPtr = std::shared_ptr<TmsServerFunctionBlock<FunctionBlockPtr>>;

template <typename T>
class TmsServerFunctionBlock : public TmsServerComponent<T>
{
public:
    using Super = TmsServerComponent<T>;

    TmsServerFunctionBlock(const FunctionBlockPtr& object, const opcua::OpcUaServerPtr& server, const ContextPtr& context, const TmsServerContextPtr& tmsContext);

    bool createOptionalNode(const opcua::OpcUaNodeId& nodeId) override;
    void bindCallbacks() override;

    void addChildNodes() override;
    void createNonhierarchicalReferences() override;

protected:
    opcua::OpcUaNodeId getTmsTypeId() override;

    // TODO we need following list to keep this becouse of handlers. Move handlers (TmsServerObject) to context and use UA_NodeTypeLifecycle
    // for deleting it
    std::list<TmsServerSignalPtr> signals;
    std::list<TmsServerInputPortPtr> inputPorts;
    std::list<TmsServerFunctionBlockPtr> functionBlocks;
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
