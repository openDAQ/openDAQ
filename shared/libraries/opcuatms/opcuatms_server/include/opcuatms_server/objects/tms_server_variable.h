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
#include "opcuatms_server/objects/tms_server_object.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

template <class CoreType>
class TmsServerVariable : public TmsServerObjectBaseImpl<CoreType>
{
public:
    using Super = TmsServerObjectBaseImpl<CoreType>;

    TmsServerVariable(const CoreType& object, const opcua::OpcUaServerPtr& server, const ContextPtr& context, const TmsServerContextPtr& tmsContext);

    opcua::OpcUaNodeId createNode(const opcua::OpcUaNodeId& parentNodeId) override;

    virtual opcua::OpcUaNodeId getDataTypeId();

protected:
    virtual void configureVariableNodeAttributes(opcua::OpcUaObject<UA_VariableAttributes>& attr);
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
