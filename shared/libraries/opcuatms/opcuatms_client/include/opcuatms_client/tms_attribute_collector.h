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
#include <opcuatms/opcuatms.h>
#include <opcuashared/opcua_attribute.h>
#include <opcuatms_client/objects/tms_client_context.h>
#include <tsl/ordered_set.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

class TmsAttributeCollector
{
public:
    TmsAttributeCollector(const CachedReferenceBrowserPtr& browser);

    tsl::ordered_set<OpcUaAttribute> collectAttributes(const OpcUaNodeId& nodeId);

private:
    void collectDeviceAttributes(const OpcUaNodeId& nodeId);
    void collectFunctionBlockAttributes(const OpcUaNodeId& nodeId);
    void collectInputPortAttributes(const OpcUaNodeId& nodeId);
    void collectSignalAttributes(const OpcUaNodeId& nodeId);
    void collectComponentAttributes(const OpcUaNodeId& nodeId);
    void collectPropertyObjectAttributes(const OpcUaNodeId& nodeId);
    void collectPropertyAttributes(const OpcUaNodeId& nodeId);
    void collectEvaluationPropertyAttributes(const OpcUaNodeId& nodeId);
    void collectBaseObjectAttributes(const OpcUaNodeId& nodeId);
    void collectMethodAttributes(const OpcUaNodeId& nodeId);
    void collectVariableBlockAttributes(const OpcUaNodeId& nodeId);

    void collectIoNode(const OpcUaNodeId& nodeId);
    void collectInputPortNode(const OpcUaNodeId& nodeId);
    void collectFunctionBlockNode(const OpcUaNodeId& nodeId);
    void collectSignalsNode(const OpcUaNodeId& nodeId);
    void collectStreamingOptionsNode(const OpcUaNodeId& nodeId);
    void collectMethodSetNode(const OpcUaNodeId& nodeId);

    bool isSubtypeOf(const OpcUaNodeId& typeId, const OpcUaNodeId& baseType);
    bool typeEquals(const OpcUaNodeId& typeId, const OpcUaNodeId& baseType);

    CachedReferenceBrowserPtr browser;
    tsl::ordered_set<OpcUaAttribute> attributes;

    static const OpcUaNodeId NodeIdBaseObjectType;
    static const OpcUaNodeId NodeIdBaseVariableType;
    static const OpcUaNodeId NodeIdDeviceType;
    static const OpcUaNodeId NodeIdFunctionBlockType;
    static const OpcUaNodeId NodeIdComponentType;
    static const OpcUaNodeId NodeIdSignalType;
    static const OpcUaNodeId NodeIdInputPortType;
    static const OpcUaNodeId NodeIdEvaluationVariableType;
    static const OpcUaNodeId NodeIdVariableBlockType;
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
