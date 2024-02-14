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
#include <opendaq/mirrored_signal_config_ptr.h>
#include <opendaq/streaming_ptr.h>
#include <opendaq/streaming_private.h>
#include <opendaq/mirrored_signal_private.h>
#include <opendaq/subscription_event_args_factory.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename... Interfaces>
class MirroredSignalBase;

using MirroredSignal = MirroredSignalBase<>;

template <typename... Interfaces>
class MirroredSignalBase : public SignalBase<IMirroredSignalConfig, IMirroredSignalPrivate, Interfaces...>
{
public:
    using Super = SignalBase<IMirroredSignalConfig, IMirroredSignalPrivate, Interfaces...>;
    using Self = MirroredSignalBase<Interfaces...>;

    explicit MirroredSignalBase(const ContextPtr& ctx,
                                const ComponentPtr& parent,
                                const StringPtr& localId,
                                const StringPtr& className = nullptr);

    virtual StringPtr onGetRemoteId() const = 0;
    virtual Bool onTriggerEvent(EventPacketPtr eventPacket) = 0;

    // IMirroredSignalConfig
    ErrCode INTERFACE_FUNC getRemoteId(IString** id) const override;
    ErrCode INTERFACE_FUNC getStreamingSources(IList** streamingConnectionStrings) override;
    ErrCode INTERFACE_FUNC setActiveStreamingSource(IString* streamingConnectionString) override;
    ErrCode INTERFACE_FUNC getActiveStreamingSource(IString** streamingConnectionString) override;
    ErrCode INTERFACE_FUNC deactivateStreaming() override;
    ErrCode INTERFACE_FUNC getOnSubscribeComplete(IEvent** event) override;
    ErrCode INTERFACE_FUNC getOnUnsubscribeComplete(IEvent** event) override;

    // IMirroredSignalPrivate
    Bool INTERFACE_FUNC triggerEvent(const EventPacketPtr& eventPacket) override;
    ErrCode INTERFACE_FUNC addStreamingSource(const StreamingPtr& streaming) override;
    ErrCode INTERFACE_FUNC removeStreamingSource(const StringPtr& streamingConnectionString) override;
    Bool INTERFACE_FUNC hasMatchingId(const StringPtr& signalId) override;
    void INTERFACE_FUNC subscribeCompleted(const StringPtr& streamingConnectionString) override;
    void INTERFACE_FUNC unsubscribeCompleted(const StringPtr& streamingConnectionString) override;
    void INTERFACE_FUNC assignDomainSignal(const SignalPtr& domainSignal) override;

    // ISignalConfig
    ErrCode INTERFACE_FUNC setDescriptor(IDataDescriptor* descriptor) override;
    ErrCode INTERFACE_FUNC setDomainSignal(ISignal* signal) override;
    ErrCode INTERFACE_FUNC setRelatedSignals(IList* signals) override;
    ErrCode INTERFACE_FUNC addRelatedSignal(ISignal* signal) override;
    ErrCode INTERFACE_FUNC removeRelatedSignal(ISignal* signal) override;
    ErrCode INTERFACE_FUNC clearRelatedSignals() override;

    // ISignal
    ErrCode INTERFACE_FUNC getStreamed(Bool* streamed) override;
    ErrCode INTERFACE_FUNC setStreamed(Bool streamed) override;

protected:
    EventPacketPtr createDataDescriptorChangedEventPacket() override;
    void onListenedStatusChanged() override;

private:
    ErrCode subscribeInternal();
    ErrCode unsubscribeInternal();

    std::vector<WeakRefPtr<IStreaming>> streamingSourcesRefs;
    WeakRefPtr<IStreaming> activeStreamingSourceRef;
    bool listened;
    bool streamed;
    EventEmitter<MirroredSignalConfigPtr, SubscriptionEventArgsPtr> onSubscribeCompleteEvent;
    EventEmitter<MirroredSignalConfigPtr, SubscriptionEventArgsPtr> onUnsubscribeCompleteEvent;

    std::mutex mirroredSignalSync;
};

template <typename... Interfaces>
MirroredSignalBase<Interfaces...>::MirroredSignalBase(const ContextPtr& ctx,
                                                      const ComponentPtr& parent,
                                                      const StringPtr& localId,
                                                      const StringPtr& className)
    : Super(ctx, nullptr, parent, localId, className)
    , activeStreamingSourceRef(nullptr)
    , listened(false)
    , streamed(true)
{
}

template <typename... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::getRemoteId(IString** id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    StringPtr signalRemoteId;
    const ErrCode errCode = wrapHandlerReturn(this, &Self::onGetRemoteId, signalRemoteId);

    *id = signalRemoteId.detach();

    return errCode;
}

template <typename... Interfaces>
Bool MirroredSignalBase<Interfaces...>::triggerEvent(const EventPacketPtr& eventPacket)
{
    Bool forwardEvent;

    const ErrCode errCode = wrapHandlerReturn(this, &Self::onTriggerEvent, forwardEvent, eventPacket);
    checkErrorInfo(errCode);

    return forwardEvent;
}


template <typename... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::addStreamingSource(const StreamingPtr& streaming)
{
    if (!streaming.assigned())
        return OPENDAQ_ERR_ARGUMENT_NULL;

    const auto connectionString = streaming.getConnectionString();

    std::scoped_lock lock(mirroredSignalSync);
    auto it = std::find_if(streamingSourcesRefs.begin(),
                           streamingSourcesRefs.end(),
                           [&connectionString](const WeakRefPtr<IStreaming>& sourceRef)
                           {
                               StreamingPtr streamingSource = sourceRef.getRef();
                               return streamingSource.assigned()
                                          ? (streamingSource.getConnectionString() == connectionString)
                                          : false;
                           });
    if (it != streamingSourcesRefs.end())
        return OPENDAQ_ERR_DUPLICATEITEM;

    streamingSourcesRefs.push_back(WeakRefPtr<IStreaming>(streaming));

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::removeStreamingSource(const StringPtr& streamingConnectionString)
{
    if (!streamingConnectionString.assigned())
        return OPENDAQ_ERR_ARGUMENT_NULL;

    std::scoped_lock lock(mirroredSignalSync);

    auto it = std::find_if(streamingSourcesRefs.begin(),
                           streamingSourcesRefs.end(),
                           [&streamingConnectionString](const WeakRefPtr<IStreaming>& sourceRef)
                           {
                               StreamingPtr streamingSource = sourceRef.getRef();
                               return streamingSource.assigned()
                                          ? (streamingSource.getConnectionString() == streamingConnectionString)
                                          : false;
                           });

    if (it != streamingSourcesRefs.end())
        streamingSourcesRefs.erase(it);
    else
        return OPENDAQ_ERR_NOTFOUND;

    auto activeStreamingSource = activeStreamingSourceRef.assigned() ? activeStreamingSourceRef.getRef() : nullptr;
    if (activeStreamingSource.assigned() &&
        streamingConnectionString == activeStreamingSource.getConnectionString())
    {
        if (listened && streamed)
        {
            ErrCode errCode = unsubscribeInternal();
            if (OPENDAQ_FAILED(errCode))
                return errCode;
        }
        activeStreamingSourceRef = nullptr;
    }

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::setDescriptor(IDataDescriptor* /*descriptor*/)
{
    return this->makeErrorInfo(OPENDAQ_ERR_INVALID_OPERATION, "Mirrored signal cannot be changed on client side");
}

template <typename... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::setDomainSignal(ISignal* /*signal*/)
{
    return this->makeErrorInfo(OPENDAQ_ERR_INVALID_OPERATION, "Mirrored signal cannot be changed on client side");
}

template <typename... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::setRelatedSignals(IList* /*signals*/)
{
    return this->makeErrorInfo(OPENDAQ_ERR_INVALID_OPERATION, "Mirrored signal cannot be changed on client side");
}

template <typename... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::addRelatedSignal(ISignal* /*signal*/)
{
    return this->makeErrorInfo(OPENDAQ_ERR_INVALID_OPERATION, "Mirrored signal cannot be changed on client side");
}

template <typename... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::removeRelatedSignal(ISignal* /*signal*/)
{
    return this->makeErrorInfo(OPENDAQ_ERR_INVALID_OPERATION, "Mirrored signal cannot be changed on client side");
}

template <typename... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::clearRelatedSignals()
{
    return this->makeErrorInfo(OPENDAQ_ERR_INVALID_OPERATION, "Mirrored signal cannot be changed on client side");
}

template <typename... Interfaces>
void MirroredSignalBase<Interfaces...>::onListenedStatusChanged()
{
    std::scoped_lock lock(mirroredSignalSync);

    bool listened = this->hasListeners();
    if (this->listened == listened)
        return;
    this->listened = listened;

    if (this->listened)
    {
        if (streamed)
            checkErrorInfo(subscribeInternal());
    }
    else
    {
        if (streamed)
            checkErrorInfo(unsubscribeInternal());
    }
}

template <typename... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::getStreamingSources(IList** streamingConnectionStrings)
{
    OPENDAQ_PARAM_NOT_NULL(streamingConnectionStrings);

    auto stringsPtr = List<IString>();

    std::scoped_lock lock(mirroredSignalSync);
    for (const auto& streamingRef : streamingSourcesRefs)
    {
        auto streamingSource = streamingRef.getRef();
        if (streamingSource.assigned())
            stringsPtr.pushBack(streamingSource.getConnectionString());
    }

    *streamingConnectionStrings = stringsPtr.detach();

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::setActiveStreamingSource(IString* streamingConnectionString)
{
    OPENDAQ_PARAM_NOT_NULL(streamingConnectionString);

    const auto connectionStringPtr = StringPtr::Borrow(streamingConnectionString);
    auto thisPtr = this->template borrowPtr<MirroredSignalConfigPtr>();

    std::scoped_lock lock(mirroredSignalSync);

    auto activeStreamingSource = activeStreamingSourceRef.assigned() ? activeStreamingSourceRef.getRef() : nullptr;
    if (activeStreamingSource.assigned() &&
        activeStreamingSource.getConnectionString() == connectionStringPtr)
        return OPENDAQ_IGNORED;

    auto it = std::find_if(streamingSourcesRefs.begin(),
                           streamingSourcesRefs.end(),
                           [&connectionStringPtr](const WeakRefPtr<IStreaming>& sourceRef)
                           {
                               StreamingPtr streamingSource = sourceRef.getRef();
                               return streamingSource.assigned()
                                          ? (streamingSource.getConnectionString() == connectionStringPtr)
                                          : false;
                           });
    if (it == streamingSourcesRefs.end())
        return OPENDAQ_ERR_NOTFOUND;

    StreamingPtr streamingSource = (*it).getRef();
    if (!streamingSource.assigned())
        return OPENDAQ_ERR_NOTFOUND;

    if (listened && streamed)
    {
        ErrCode errCode = unsubscribeInternal();
        if (OPENDAQ_FAILED(errCode))
            return errCode;
    }

    activeStreamingSourceRef = streamingSource;
    if (listened && streamed)
    {
        ErrCode errCode = subscribeInternal();
        if (OPENDAQ_FAILED(errCode))
            return errCode;
    }

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::getActiveStreamingSource(IString** streamingConnectionString)
{
    OPENDAQ_PARAM_NOT_NULL(streamingConnectionString);

    std::scoped_lock lock(mirroredSignalSync);
    auto activeStreamingSource = activeStreamingSourceRef.assigned() ? activeStreamingSourceRef.getRef() : nullptr;
    if (activeStreamingSource.assigned())
        *streamingConnectionString = activeStreamingSource.getConnectionString().addRefAndReturn();
    else
        *streamingConnectionString = nullptr;

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::deactivateStreaming()
{
    auto thisPtr = this->template borrowPtr<MirroredSignalConfigPtr>();

    std::scoped_lock lock(mirroredSignalSync);

    ErrCode errCode = OPENDAQ_SUCCESS;
    if (listened && streamed)
        errCode = unsubscribeInternal();
    activeStreamingSourceRef = nullptr;

    if (OPENDAQ_FAILED(errCode))
        return errCode;

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::getOnSubscribeComplete(IEvent** event)
{
    OPENDAQ_PARAM_NOT_NULL(event);

    *event = onSubscribeCompleteEvent.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::getOnUnsubscribeComplete(IEvent** event)
{
    OPENDAQ_PARAM_NOT_NULL(event);

    *event = onUnsubscribeCompleteEvent.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
EventPacketPtr MirroredSignalBase<Interfaces...>::createDataDescriptorChangedEventPacket()
{
    std::scoped_lock lock(mirroredSignalSync);

    auto activeStreamingSource = activeStreamingSourceRef.assigned() ? activeStreamingSourceRef.getRef() : nullptr;
    if (activeStreamingSource.assigned())
    {
        auto thisPtr = this->template borrowPtr<MirroredSignalConfigPtr>();
        return activeStreamingSource.template asPtr<IStreamingPrivate>()->createDataDescriptorChangedEventPacket(thisPtr);
    }

    return DataDescriptorChangedEventPacket(nullptr, nullptr);
}

template <typename... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::subscribeInternal()
{
    auto activeStreamingSource = activeStreamingSourceRef.assigned() ? activeStreamingSourceRef.getRef() : nullptr;
    if (activeStreamingSource.assigned())
    {
        auto thisPtr = this->template borrowPtr<MirroredSignalConfigPtr>();
        return activeStreamingSource.template asPtr<IStreamingPrivate>()->subscribeSignal(thisPtr);
    }
    else
    {
        return OPENDAQ_IGNORED;
    }
}

template <typename... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::unsubscribeInternal()
{
    auto activeStreamingSource = activeStreamingSourceRef.assigned() ? activeStreamingSourceRef.getRef() : nullptr;
    if (activeStreamingSource.assigned())
    {
        auto thisPtr = this->template borrowPtr<MirroredSignalConfigPtr>();
        return activeStreamingSource.template asPtr<IStreamingPrivate>()->unsubscribeSignal(thisPtr);
    }
    else
    {
        return OPENDAQ_IGNORED;
    }
}

template <typename... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::getStreamed(Bool* streamed)
{
    OPENDAQ_PARAM_NOT_NULL(streamed);

    std::scoped_lock lock(mirroredSignalSync);
    *streamed = this->streamed;
    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::setStreamed(Bool streamed)
{
    std::scoped_lock lock(mirroredSignalSync);

    if (static_cast<bool>(streamed) == this->streamed)
        return OPENDAQ_IGNORED;

    this->streamed = streamed;

    ErrCode errCode = OPENDAQ_SUCCESS;
    if (this->streamed)
    {
        if (listened)
            errCode = subscribeInternal();
    }
    else
    {
        if (listened)
            errCode = unsubscribeInternal();
    }
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
Bool MirroredSignalBase<Interfaces...>::hasMatchingId(const StringPtr& signalId)
{
    StringPtr signalRemoteId;
    const ErrCode errCode = wrapHandlerReturn(this, &Self::onGetRemoteId, signalRemoteId);
    checkErrorInfo(errCode);

    if (signalId.getLength() > signalRemoteId.getLength())
        return False;

    std::string idEnding = signalId.toStdString();
    std::string fullId = signalRemoteId.toStdString();
    return std::equal(idEnding.rbegin(), idEnding.rend(), fullId.rbegin());
}

template <typename... Interfaces>
void MirroredSignalBase<Interfaces...>::subscribeCompleted(const StringPtr& streamingConnectionString)
{
    auto thisPtr = this->template borrowPtr<MirroredSignalConfigPtr>();
    if (onSubscribeCompleteEvent.hasListeners())
    {
        onSubscribeCompleteEvent(
            thisPtr,
            SubscriptionEventArgs(streamingConnectionString, SubscriptionEventType::Subscribed)
        );
    }
}

template <typename... Interfaces>
void MirroredSignalBase<Interfaces...>::unsubscribeCompleted(const StringPtr& streamingConnectionString)
{
    auto thisPtr = this->template borrowPtr<MirroredSignalConfigPtr>();
    if (onUnsubscribeCompleteEvent.hasListeners())
    {
        onUnsubscribeCompleteEvent(
            thisPtr,
            SubscriptionEventArgs(streamingConnectionString, SubscriptionEventType::Unsubscribed)
        );
    }
}

template <typename... Interfaces>
void MirroredSignalBase<Interfaces...>::assignDomainSignal(const SignalPtr& domainSignal)
{
    if (domainSignal.asPtrOrNull<IMirroredSignalConfig>() == nullptr)
    {
        throw NoInterfaceException(
            fmt::format(R"(Domain signal "{}" does not implement IMirroredSignalConfig interface.)",
                        domainSignal.getGlobalId()));
    }

    ErrCode errCode = Super::setDomainSignal(domainSignal);
    checkErrorInfo(errCode);
}

END_NAMESPACE_OPENDAQ
