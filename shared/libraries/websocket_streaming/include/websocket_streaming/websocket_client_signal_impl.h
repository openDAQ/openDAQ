/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <opendaq/mirrored_signal_impl.h>

#include "websocket_streaming/websocket_streaming.h"

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

class WebsocketClientSignalImpl final : public MirroredSignal
{
public:
    explicit WebsocketClientSignalImpl(const ContextPtr& ctx,
                                       const ComponentPtr& parent,
                                       const StringPtr& streamingId);

    StringPtr onGetRemoteId() const override;
    Bool onTriggerEvent(const EventPacketPtr& eventPacket) override;

protected:
    SignalPtr onGetDomainSignal() override;
    DataDescriptorPtr onGetDescriptor() override;

private:
    static StringPtr CreateLocalId(const StringPtr& streamingId);

    StringPtr streamingId;
};

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
