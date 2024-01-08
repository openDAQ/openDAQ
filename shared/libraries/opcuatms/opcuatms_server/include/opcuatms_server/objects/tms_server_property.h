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
#include "property_internal_ptr.h"
#include "coreobjects/property_ptr.h"
#include "coretypes/listobject.h"
#include "opcuatms_server/objects/tms_server_eval_value.h"
#include "opcuatms_server/objects/tms_server_object.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

class TmsServerProperty;
using TmsServerPropertyPtr = std::shared_ptr<TmsServerProperty>;

class TmsServerProperty : public TmsServerVariable<PropertyPtr>
{
public:
    using Super = TmsServerVariable<PropertyPtr>;
    
    TmsServerProperty(const PropertyPtr& object,
                      const opcua::OpcUaServerPtr& server,
                      const ContextPtr& context,
                      const TmsServerContextPtr& tmsContext);
    TmsServerProperty(const PropertyPtr& object,
                      const opcua::OpcUaServerPtr& server,
                      const ContextPtr& context,
                      const TmsServerContextPtr& tmsContext,
                      const std::unordered_map<std::string, uint32_t>& propOrder);
    TmsServerProperty(const PropertyPtr& object,
                      const opcua::OpcUaServerPtr& server,
                      const ContextPtr& context,
                      const TmsServerContextPtr& tmsContext,
                      const PropertyObjectPtr& parent,
                      const std::unordered_map<std::string, uint32_t>& propOrder);

    std::string getBrowseName() override;
    void bindCallbacks() override;

protected:
    opcua::OpcUaNodeId getTmsTypeId() override;
    bool createOptionalNode(const opcua::OpcUaNodeId& nodeId) override;
    void addChildNodes() override;
    void configureVariableNodeAttributes(opcua::OpcUaObject<UA_VariableAttributes>& attr) override;
    void validate() override;

private:
    void registerEvalValueNode(const std::string& nodeName, TmsServerEvalValue::ReadCallback readCallback, bool isSelection = false);
    bool isSelectionType();
    bool isNumericType();
    bool isIntrospectionType();
    bool isReferenceType();
    bool isStructureType();

    void hideReferenceTypeChildren();
    void hideNumericTypeChildren();
    void hideSelectionTypeChildren();
    void hideIntrospectionTypeChildren();
    void hideStructureTypeChildren();

    void addReferenceTypeChildNodes();
    void addNumericTypeChildNodes();
    void addSelectionTypeChildNodes();
    void addIntrospectionTypeChildNodes();
    
    std::unordered_map<opcua::OpcUaNodeId, TmsServerObjectPtr> childObjects;
    PropertyInternalPtr objectInternal;
    WeakRefPtr<IPropertyObject> parent;

    std::unordered_set<std::string> HiddenNodes = {"FieldCoercionExpression", "FieldValidationExpression", "<BaseDataVariable>"};
    std::unordered_map<opcua::OpcUaNodeId, TmsServerPropertyPtr> childProperties;
    std::unordered_map<std::string, uint32_t> propOrder;
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
