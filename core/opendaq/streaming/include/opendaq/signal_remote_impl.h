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
#include <opendaq/signal_impl.h>
#include <opendaq/signal_remote.h>
#include <opendaq/streaming_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

template <SignalStandardProps Props, typename... Interfaces>
class SignalRemoteBase;

template <SignalStandardProps Props>
using SignalRemote = SignalRemoteBase<Props>;

template <SignalStandardProps Props, typename... Interfaces>
class SignalRemoteBase : public SignalBase<Props, ISignalRemote, Interfaces...>
{
public:
    using Super = SignalBase<Props, ISignalRemote, Interfaces...>;
    using Self = SignalRemoteBase<Props, Interfaces...>;

    explicit SignalRemoteBase(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);

    virtual StringPtr onGetRemoteId() const = 0;
    virtual Bool onTriggerEvent(EventPacketPtr eventPacket) = 0;

    // ISignalRemote
    ErrCode INTERFACE_FUNC getRemoteId(IString** id) const override;
    ErrCode INTERFACE_FUNC triggerEvent(IEventPacket* eventPacket, Bool* forwardEvent) override;
    ErrCode INTERFACE_FUNC addStreamingSource(IStreaming* streaming) override;
    ErrCode INTERFACE_FUNC removeStreamingSource(IStreaming* streaming) override;

    // ISignalConfig
    ErrCode INTERFACE_FUNC setDescriptor(IDataDescriptor* descriptor) override;
    ErrCode INTERFACE_FUNC setDomainSignal(ISignal* signal) override;
    ErrCode INTERFACE_FUNC setRelatedSignals(IList* signals) override;
    ErrCode INTERFACE_FUNC addRelatedSignal(ISignal* signal) override;
    ErrCode INTERFACE_FUNC removeRelatedSignal(ISignal* signal) override;
    ErrCode INTERFACE_FUNC clearRelatedSignals() override;
};

template <SignalStandardProps Props, typename... Interfaces>
SignalRemoteBase<Props, Interfaces...>::SignalRemoteBase(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : Super(ctx, parent, localId)
{
}

template <SignalStandardProps Props, typename... Interfaces>
ErrCode SignalRemoteBase<Props, Interfaces...>::getRemoteId(IString** id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    StringPtr signalRemoteId;
    const ErrCode errCode = wrapHandlerReturn(this, &Self::onGetRemoteId, signalRemoteId);

    *id = signalRemoteId.detach();

    return errCode;
}

template <SignalStandardProps Props, typename... Interfaces>
ErrCode SignalRemoteBase<Props, Interfaces...>::triggerEvent(IEventPacket* eventPacket, Bool* forwardEvent)
{
    OPENDAQ_PARAM_NOT_NULL(eventPacket);
    OPENDAQ_PARAM_NOT_NULL(forwardEvent);

    const auto eventPacketPtr = EventPacketPtr::Borrow(eventPacket);
    const ErrCode errCode = wrapHandlerReturn(this, &Self::onTriggerEvent, *forwardEvent, eventPacketPtr);
    return errCode;
}

template <SignalStandardProps Props, typename... Interfaces>
ErrCode SignalRemoteBase<Props, Interfaces...>::addStreamingSource(IStreaming* streaming)
{
    OPENDAQ_PARAM_NOT_NULL(streaming);

    const auto streamingPtr = StreamingPtr::Borrow(streaming);
    auto connectionString = streamingPtr.getConnectionString();

    std::scoped_lock lock(this->sync);
    auto it = std::find(this->streamingSources.begin(), this->streamingSources.end(), connectionString);
    if (it != this->streamingSources.end())
        return OPENDAQ_ERR_DUPLICATEITEM;

    this->streamingSources.push_back(connectionString);
    return OPENDAQ_SUCCESS;
}

template <SignalStandardProps Props, typename... Interfaces>
ErrCode SignalRemoteBase<Props, Interfaces...>::removeStreamingSource(IStreaming* streaming)
{
    OPENDAQ_PARAM_NOT_NULL(streaming);

    const auto streamingPtr = StreamingPtr::Borrow(streaming);
    auto connectionString = streamingPtr.getConnectionString();

    std::scoped_lock lock(this->sync);
    auto it = std::find(this->streamingSources.begin(), this->streamingSources.end(), connectionString);
    if (it == this->streamingSources.end())
        return OPENDAQ_ERR_NOTFOUND;

    this->streamingSources.erase(it);
    if (connectionString == this->activeStreamingSource)
        this->activeStreamingSource = nullptr;
    return OPENDAQ_SUCCESS;
}

template <SignalStandardProps Props, typename... Interfaces>
ErrCode SignalRemoteBase<Props, Interfaces...>::setDescriptor(IDataDescriptor* /*descriptor*/)
{
    return this->makeErrorInfo(OPENDAQ_ERR_INVALID_OPERATION, "Mirrored signal cannot be changed on client side");
}

template <SignalStandardProps Props, typename... Interfaces>
ErrCode SignalRemoteBase<Props, Interfaces...>::setDomainSignal(ISignal* /*signal*/)
{
    return this->makeErrorInfo(OPENDAQ_ERR_INVALID_OPERATION, "Mirrored signal cannot be changed on client side");
}

template <SignalStandardProps Props, typename... Interfaces>
ErrCode SignalRemoteBase<Props, Interfaces...>::setRelatedSignals(IList* /*signals*/)
{
    return this->makeErrorInfo(OPENDAQ_ERR_INVALID_OPERATION, "Mirrored signal cannot be changed on client side");
}

template <SignalStandardProps Props, typename... Interfaces>
ErrCode SignalRemoteBase<Props, Interfaces...>::addRelatedSignal(ISignal* /*signal*/)
{
    return this->makeErrorInfo(OPENDAQ_ERR_INVALID_OPERATION, "Mirrored signal cannot be changed on client side");
}

template <SignalStandardProps Props, typename... Interfaces>
ErrCode SignalRemoteBase<Props, Interfaces...>::removeRelatedSignal(ISignal* /*signal*/)
{
    return this->makeErrorInfo(OPENDAQ_ERR_INVALID_OPERATION, "Mirrored signal cannot be changed on client side");
}

template <SignalStandardProps Props, typename... Interfaces>
ErrCode SignalRemoteBase<Props, Interfaces...>::clearRelatedSignals()
{
    return this->makeErrorInfo(OPENDAQ_ERR_INVALID_OPERATION, "Mirrored signal cannot be changed on client side");
}

END_NAMESPACE_OPENDAQ
