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
#include <opcuatms_client/objects/tms_client_component_impl.h>
#include <opendaq/input_port_impl.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

class TmsClientInputPortImpl : public TmsClientComponentBaseImpl<GenericInputPortImpl<ITmsClientComponent>>
{
public:
    explicit TmsClientInputPortImpl(const ContextPtr& ctx,
                                    const ComponentPtr& parent,
                                    const StringPtr& localId,
                                    const TmsClientContextPtr& tmsCtx,
                                    const opcua::OpcUaNodeId& nodeId);

    ErrCode INTERFACE_FUNC acceptsSignal(ISignal* signal, Bool* accepts) override;
    ErrCode INTERFACE_FUNC connect(ISignal* signal) override;
    ErrCode INTERFACE_FUNC disconnect() override;
    ErrCode INTERFACE_FUNC getSignal(ISignal** signal) override;
    ErrCode INTERFACE_FUNC getConnection(IConnection** connection) override;
    ErrCode INTERFACE_FUNC getRequiresSignal(Bool* value) override;
    ErrCode INTERFACE_FUNC setRequiresSignal(Bool value) override;

protected:
    SignalPtr onGetSignal();
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
