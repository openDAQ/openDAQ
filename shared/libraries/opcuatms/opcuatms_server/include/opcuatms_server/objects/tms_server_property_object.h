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
#include "coreobjects/property_object_ptr.h"
#include "opcuatms_server/objects/tms_server_object.h"
#include "opcuatms_server/objects/tms_server_property.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

class TmsServerPropertyObject;
using TmsServerPropertyObjectPtr = std::shared_ptr<TmsServerPropertyObject>;

class TmsServerPropertyObject : public TmsServerObjectBaseImpl<PropertyObjectPtr>
{
public:
    using Super = TmsServerObjectBaseImpl<PropertyObjectPtr>;

    TmsServerPropertyObject(const PropertyObjectPtr& object,
                            const opcua::OpcUaServerPtr& server,
                            const ContextPtr& context,
                            const TmsServerContextPtr& tmsContext,
                            const std::unordered_set<std::string>& ignoredProps = {});
    TmsServerPropertyObject(const PropertyObjectPtr& object,
                            const opcua::OpcUaServerPtr& server,
                            const ContextPtr& context,
                            const TmsServerContextPtr& tmsContext,
                            const StringPtr& name);
    TmsServerPropertyObject(const PropertyObjectPtr& object,
                            const opcua::OpcUaServerPtr& server,
                            const ContextPtr& context,
                            const TmsServerContextPtr& tmsContext,
                            const StringPtr& name,
                            const PropertyPtr& objProp);
    ~TmsServerPropertyObject();
    
    std::string getBrowseName() override;
    void addChildNodes() override;
    void bindCallbacks() override;
    bool createOptionalNode(const opcua::OpcUaNodeId& nodeId) override;
    void setMethodParentNodeId(const opcua::OpcUaNodeId& methodParentNodeId);
    std::unordered_set<std::string> ignoredProps;

protected:
    void configureNodeAttributes(opcua::OpcUaObject<UA_ObjectAttributes>& attr) override;
    void triggerEvent(PropertyObjectPtr& sender, PropertyValueEventArgsPtr& args);
    opcua::OpcUaNodeId getTmsTypeId() override;
    void addPropertyNode(const std::string& name, const opcua::OpcUaNodeId& parentId);
    void bindPropertyCallbacks(const std::string& name);

    std::unordered_map<opcua::OpcUaNodeId, TmsServerPropertyPtr> childProperties;
    std::unordered_map<opcua::OpcUaNodeId, TmsServerPropertyObjectPtr> childObjects;
    std::unordered_map<opcua::OpcUaNodeId, TmsServerObjectPtr> childEvalValues;
    std::unordered_map<opcua::OpcUaNodeId, std::pair<std::string, CoreType>> methodProps;

    void registerEvalValueNode(const std::string& nodeName, TmsServerEvalValue::ReadCallback readCallback);
    void addMethodPropertyNode(const PropertyPtr& prop, uint32_t numberInList);
    void bindMethodCallbacks();
    void addBeginUpdateNode();
    void addEndUpdateNode();

    StringPtr name;
    PropertyInternalPtr objProp;
    opcua::OpcUaNodeId methodParentNodeId;
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
