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
#include "opendaq/mirrored_signal_impl.h"
#include "opcuatms_client/objects/tms_client_component_impl.h"
#include "opendaq/data_descriptor_ptr.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

// TmsClientSignalImpl

class TmsClientSignalImpl final : public TmsClientComponentBaseImpl<MirroredSignal>
{
public:
    explicit TmsClientSignalImpl(const ContextPtr& ctx,
                                 const ComponentPtr& parent,
                                 const StringPtr& localId,
                                 const TmsClientContextPtr& clientContext,
                                 const opcua::OpcUaNodeId& nodeId);

    ErrCode INTERFACE_FUNC getPublic(Bool* active) override;
    ErrCode INTERFACE_FUNC setPublic(Bool active) override;

    ErrCode INTERFACE_FUNC getDescriptor(IDataDescriptor** descriptor) override;
    ErrCode INTERFACE_FUNC setDescriptor(IDataDescriptor* descriptor) override;

    ErrCode INTERFACE_FUNC getDomainSignal(ISignal** signal) override;
    SignalPtr onGetDomainSignal();
    ErrCode INTERFACE_FUNC setDomainSignal(ISignal* signal) override;

    ErrCode INTERFACE_FUNC getRelatedSignals(IList** signals) override;
    ListPtr<ISignal> onGetRelatedSignals();
    ErrCode INTERFACE_FUNC setRelatedSignals(IList* signals) override;
    ErrCode INTERFACE_FUNC addRelatedSignal(ISignal* signal) override;
    ErrCode INTERFACE_FUNC removeRelatedSignal(ISignal* signal) override;
    ErrCode INTERFACE_FUNC clearRelatedSignals() override;

    StringPtr onGetRemoteId() const override;
    Bool onTriggerEvent(EventPacketPtr eventPacket) override;

protected:
    std::atomic<Bool> isPublic = true;
    std::string deviceSignalId;

private:
    std::unique_ptr<opcua::OpcUaNodeId> descriptorNodeId;
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
