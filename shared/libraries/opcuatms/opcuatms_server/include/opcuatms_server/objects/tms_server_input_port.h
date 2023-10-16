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
#include <opendaq/input_port_ptr.h>
#include "opcuatms_server/objects/tms_server_component.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

class TmsServerInputPort;
using TmsServerInputPortPtr = std::shared_ptr<TmsServerInputPort>;

class TmsServerInputPort : public TmsServerComponent<InputPortPtr>
{
public:
    using Super = TmsServerComponent<InputPortPtr>;

    TmsServerInputPort(const InputPortPtr& object, const opcua::OpcUaServerPtr& server, const ContextPtr& context);

    opcua::OpcUaNodeId getReferenceType() override;
    void bindCallbacks() override;
    void createNonhierarchicalReferences() override;

protected:
    opcua::OpcUaNodeId getTmsTypeId() override;
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
