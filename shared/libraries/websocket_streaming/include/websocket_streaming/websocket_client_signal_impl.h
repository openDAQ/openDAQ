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
#include <opendaq/signal_remote_impl.h>

#include "websocket_streaming/websocket_streaming.h"

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

DECLARE_OPENDAQ_INTERFACE(IWebsocketStreamingSignalPrivate, IBaseObject)
{
    virtual void assignDomainSignal(const DataDescriptorPtr& domainDescriptor) = 0;
    virtual void assignDescriptor(const DataDescriptorPtr& descriptor) = 0;
};

class WebsocketClientSignalImpl final : public SignalRemoteBase<SignalStandardProps::AddReadOnly, IWebsocketStreamingSignalPrivate>
{
public:
    explicit WebsocketClientSignalImpl(const ContextPtr& ctx,
                                       const ComponentPtr& parent,
                                       const StringPtr& streamingId);

    // ISignal
    ErrCode INTERFACE_FUNC getDescriptor(IDataDescriptor** descriptor) override;
    ErrCode INTERFACE_FUNC getDomainSignal(ISignal** signal) override;

    StringPtr onGetRemoteId() const override;
    Bool onTriggerEvent(EventPacketPtr eventPacket) override;

    void assignDomainSignal(const DataDescriptorPtr& domainDescriptor) override;
    void assignDescriptor(const DataDescriptorPtr& descriptor) override;

protected:
    EventPacketPtr createDataDescriptorChangedEventPacket() override;

private:
    static StringPtr CreateLocalId(const StringPtr& streamingId);

    StringPtr streamingId;
    DataDescriptorPtr mirroredDataDescriptor;
    SignalConfigPtr domainSignalArtificial;
};

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
