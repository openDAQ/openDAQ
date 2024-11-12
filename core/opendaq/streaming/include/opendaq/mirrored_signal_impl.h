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
#include <opendaq/signal_impl.h>
#include <opendaq/mirrored_signal_config_ptr.h>
#include <opendaq/streaming_ptr.h>
#include <opendaq/streaming_private.h>
#include <opendaq/mirrored_signal_private_ptr.h>
#include <opendaq/subscription_event_args_factory.h>

#include <opendaq/event_packet_utils.h>

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
    virtual Bool onTriggerEvent(const EventPacketPtr& eventPacket);

    // IMirroredSignalConfig
    ErrCode INTERFACE_FUNC getRemoteId(IString** id) const override;
    ErrCode INTERFACE_FUNC getStreamingSources(IList** streamingConnectionStrings) override;
    ErrCode INTERFACE_FUNC setActiveStreamingSource(IString* streamingConnectionString) override;
    ErrCode INTERFACE_FUNC getActiveStreamingSource(IString** streamingConnectionString) override;
    ErrCode INTERFACE_FUNC deactivateStreaming() override;
    ErrCode INTERFACE_FUNC getOnSubscribeComplete(IEvent** event) override;
    ErrCode INTERFACE_FUNC getOnUnsubscribeComplete(IEvent** event) override;
    
    // IMirroredSignalPrivate
    ErrCode INTERFACE_FUNC triggerEvent(IEventPacket* eventPacket, Bool* forward) override;
    ErrCode INTERFACE_FUNC addStreamingSource(IStreaming* streaming) override;
    ErrCode INTERFACE_FUNC removeStreamingSource(IString* streamingConnectionString) override;
    ErrCode INTERFACE_FUNC subscribeCompleted(IString* streamingConnectionString) override;
    ErrCode INTERFACE_FUNC unsubscribeCompleted(IString* streamingConnectionString) override;
    ErrCode INTERFACE_FUNC unsubscribeCompletedNoLock(IString* streamingConnectionString) override;
    ErrCode INTERFACE_FUNC getMirroredDataDescriptor(IDataDescriptor** descriptor) override;
    ErrCode INTERFACE_FUNC setMirroredDataDescriptor(IDataDescriptor* descriptor) override;
    ErrCode INTERFACE_FUNC getMirroredDomainSignal(IMirroredSignalConfig** domainSignals) override;
    ErrCode INTERFACE_FUNC setMirroredDomainSignal(IMirroredSignalConfig* domainSignal) override;

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
    ErrCode INTERFACE_FUNC getSignalSerializeId(IString** serializeId) override;

protected:
    EventPacketPtr createDataDescriptorChangedEventPacket() override;
    void onListenedStatusChanged(bool listened) override;
    void removed() override;
    virtual bool clearDescriptorOnUnsubscribe();

    std::mutex signalMutex;

    DataDescriptorPtr mirroredDataDescriptor;
    DataDescriptorPtr mirroredDomainDataDescriptor;
    MirroredSignalConfigPtr mirroredDomainSignal;

private:
    ErrCode subscribeInternal();
    ErrCode unsubscribeInternal();
    ErrCode unsubscribeCompletedInternal(IString* streamingConnectionString, bool syncLock);

    // vector is used as the order of adding & accessing sources is important
    // store a pair string + weak reference to manage the removal of destroyed sources
    std::vector<std::pair<StringPtr, WeakRefPtr<IStreaming>>> streamingSourcesRefs;
    WeakRefPtr<IStreaming> activeStreamingSourceRef;
    bool listened;
    bool streamed;
    EventEmitter<MirroredSignalConfigPtr, SubscriptionEventArgsPtr> onSubscribeCompleteEvent;
    EventEmitter<MirroredSignalConfigPtr, SubscriptionEventArgsPtr> onUnsubscribeCompleteEvent;

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

template <typename ... Interfaces>
Bool MirroredSignalBase<Interfaces...>::onTriggerEvent(const EventPacketPtr& eventPacket)
{
    if (!eventPacket.assigned())
        return False;

    if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        const auto [signalDescriptorChanged, domainDescriptorChanged, newSignalDescriptor, newDomainDescriptor] =
            parseDataDescriptorEventPacket(eventPacket);

        std::scoped_lock lock(signalMutex);

        Bool changed = False;

        if (signalDescriptorChanged && newSignalDescriptor != mirroredDataDescriptor)
        {
            mirroredDataDescriptor = newSignalDescriptor;
            changed = True;
        }

        if (domainDescriptorChanged && mirroredDomainDataDescriptor != newDomainDescriptor)
        {
            mirroredDomainDataDescriptor = newDomainDescriptor;

            if (mirroredDomainSignal.assigned())
            {
                const auto domainSignalEventPacket = DataDescriptorChangedEventPacket(newDomainDescriptor, nullptr);
                mirroredDomainSignal.asPtr<IMirroredSignalPrivate>().triggerEvent(domainSignalEventPacket);
            }

            changed = True;
        }

        return changed;
    }

    // packet was not handled so returns True to forward the original packet
    return True;
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
void MirroredSignalBase<Interfaces...>::removed()
{
    if (listened && streamed)
        unsubscribeInternal();
    activeStreamingSourceRef = nullptr;

    StringPtr signalRemoteId;
    ErrCode errCode = wrapHandlerReturn(this, &Self::onGetRemoteId, signalRemoteId);
    if (OPENDAQ_SUCCEEDED(errCode) && signalRemoteId.assigned())
    {
        for (const auto& [_, streamingRef] : streamingSourcesRefs)
        {
            if (auto streamingSource = streamingRef.getRef(); streamingSource.assigned())
                streamingSource.template asPtr<IStreamingPrivate>()->detachRemovedSignal(signalRemoteId);
        }
        streamingSourcesRefs.clear();
    }
    Super::removed();
}

template <typename ... Interfaces>
bool MirroredSignalBase<Interfaces...>::clearDescriptorOnUnsubscribe()
{
    return false;
}

template <typename... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::triggerEvent(IEventPacket* eventPacket, Bool* forward)
{
    Bool forwardEvent;

    const ErrCode errCode = wrapHandlerReturn(this, &Self::onTriggerEvent, forwardEvent, eventPacket);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    *forward = forwardEvent;
    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::addStreamingSource(IStreaming* streaming)
{
    OPENDAQ_PARAM_NOT_NULL(streaming);

    const auto streamingPtr = StreamingPtr::Borrow(streaming);
    const auto connectionString = streamingPtr.getConnectionString();

    auto lock = this->getRecursiveConfigLock();

    auto it = std::find_if(streamingSourcesRefs.begin(),
                           streamingSourcesRefs.end(),
                           [&connectionString](const std::pair<StringPtr, WeakRefPtr<IStreaming>>& item)
                           {
                               return connectionString == item.first;
                           });

    if (it != streamingSourcesRefs.end())
    {
        return this->makeErrorInfo(
            OPENDAQ_ERR_DUPLICATEITEM,
            fmt::format(
                R"(Signal with global Id "{}" already has streaming source "{}" )",
                this->globalId,
                connectionString
            )
        );
    }

    streamingSourcesRefs.push_back({connectionString, WeakRefPtr<IStreaming>(streamingPtr)});
    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::removeStreamingSource(IString* streamingConnectionString)
{
    OPENDAQ_PARAM_NOT_NULL(streamingConnectionString);

    auto lock = this->getRecursiveConfigLock();

    const auto streamingConnectionStringPtr = StringPtr::Borrow(streamingConnectionString);

    auto it = std::find_if(streamingSourcesRefs.begin(),
                           streamingSourcesRefs.end(),
                           [&streamingConnectionStringPtr](const std::pair<StringPtr, WeakRefPtr<IStreaming>>& item)
                           {
                               return streamingConnectionStringPtr == item.first;
                           });

    if (it != streamingSourcesRefs.end())
    {
        streamingSourcesRefs.erase(it);
    }
    else
    {
        return this->makeErrorInfo(
            OPENDAQ_ERR_NOTFOUND,
            fmt::format(
                R"(Signal with global Id "{}" does not have streaming source "{}" )",
                this->globalId,
                streamingConnectionStringPtr
            )
        );
    }

    if (activeStreamingSourceRef.assigned())
    {
        auto activeStreamingSource = activeStreamingSourceRef.getRef();
        if (!activeStreamingSource.assigned())
        {
            // source is already destroyed
            activeStreamingSourceRef = nullptr;
        }
        else if (streamingConnectionStringPtr == activeStreamingSource.getConnectionString())
        {
            if (listened && streamed)
            {
                ErrCode errCode = unsubscribeInternal();
                if (OPENDAQ_FAILED(errCode))
                    return errCode;
            }
            activeStreamingSourceRef = nullptr;
        }
    }

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::subscribeCompleted(IString* streamingConnectionString)
{
    OPENDAQ_PARAM_NOT_NULL(streamingConnectionString);

    const auto streamingConnectionStringPtr = StringPtr::Borrow(streamingConnectionString);
    auto thisPtr = this->template borrowPtr<MirroredSignalConfigPtr>();
    if (onSubscribeCompleteEvent.hasListeners())
    {
        onSubscribeCompleteEvent(
            thisPtr,
            SubscriptionEventArgs(streamingConnectionStringPtr, SubscriptionEventType::Subscribed)
        );
    }

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::unsubscribeCompleted(IString* streamingConnectionString)
{
    return unsubscribeCompletedInternal(streamingConnectionString, true);
}

template <typename... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::unsubscribeCompletedNoLock(IString* streamingConnectionString)
{
    return unsubscribeCompletedInternal(streamingConnectionString, false);
}

template <typename... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::unsubscribeCompletedInternal(IString* streamingConnectionString, bool syncLock)
{
    OPENDAQ_PARAM_NOT_NULL(streamingConnectionString);

    const auto streamingConnectionStringPtr = StringPtr::Borrow(streamingConnectionString);
    auto thisPtr = this->template borrowPtr<MirroredSignalConfigPtr>();

    if (clearDescriptorOnUnsubscribe())
    {
        std::scoped_lock lock(signalMutex);
        mirroredDataDescriptor = nullptr;
        mirroredDomainDataDescriptor = nullptr;
    }

    if (syncLock)
    {
        auto lock = this->getRecursiveConfigLock();
        this->lastDataValue = nullptr;
    }
    else
    {
        this->lastDataValue = nullptr;
    }

    if (onUnsubscribeCompleteEvent.hasListeners())
    {
        onUnsubscribeCompleteEvent(thisPtr, SubscriptionEventArgs(streamingConnectionStringPtr, SubscriptionEventType::Unsubscribed));
    }

    return OPENDAQ_SUCCESS;
}

template <typename ... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::getMirroredDataDescriptor(IDataDescriptor** descriptor)
{
    OPENDAQ_PARAM_NOT_NULL(descriptor);

    std::scoped_lock lock(signalMutex);
    *descriptor = mirroredDataDescriptor.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <typename ... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::setMirroredDataDescriptor(IDataDescriptor* descriptor)
{
    std::scoped_lock lock(signalMutex);
    mirroredDataDescriptor = descriptor;
    return OPENDAQ_SUCCESS;
}

template <typename ... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::getMirroredDomainSignal(IMirroredSignalConfig** domainSignal)
{
    OPENDAQ_PARAM_NOT_NULL(domainSignal);

    std::scoped_lock lock(signalMutex);
    *domainSignal = mirroredDomainSignal.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <typename ... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::setMirroredDomainSignal(IMirroredSignalConfig* domainSignal)
{
    std::scoped_lock lock(signalMutex);
    mirroredDomainSignal = domainSignal;

    if (domainSignal)
    {
        const ErrCode err = mirroredDomainSignal.asPtr<IMirroredSignalPrivate>()->getMirroredDataDescriptor(&mirroredDomainDataDescriptor);
        if (OPENDAQ_FAILED(err))
            return err;
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
void MirroredSignalBase<Interfaces...>::onListenedStatusChanged(bool listened)
{
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

    auto lock = this->getRecursiveConfigLock();
    for (const auto& [connectionString, streamingRef] : streamingSourcesRefs)
    {
        auto streamingSource = streamingRef.getRef();
        if (streamingSource.assigned())
            stringsPtr.pushBack(connectionString);
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

    auto lock = this->getRecursiveConfigLock();

    auto activeStreamingSource = activeStreamingSourceRef.assigned() ? activeStreamingSourceRef.getRef() : nullptr;
    if (activeStreamingSource.assigned() &&
        activeStreamingSource.getConnectionString() == connectionStringPtr)
        return OPENDAQ_IGNORED;

    auto it = std::find_if(streamingSourcesRefs.begin(),
                           streamingSourcesRefs.end(),
                           [&connectionStringPtr](const std::pair<StringPtr, WeakRefPtr<IStreaming>>& item)
                           {
                               return connectionStringPtr == item.first;
                           });

    if (it == streamingSourcesRefs.end())
    {
        return this->makeErrorInfo(
            OPENDAQ_ERR_NOTFOUND,
            fmt::format(
                R"(Signal with global Id "{}" does not have streaming source "{}" )",
                this->globalId,
                connectionStringPtr
            )
        );
    }

    StreamingPtr streamingSource = it->second.getRef();
    if (!streamingSource.assigned())
    {
        return this->makeErrorInfo(
            OPENDAQ_ERR_NOTFOUND,
            fmt::format(
                R"(Signal with global Id "{}": streaming source "{}" is already destroyed)",
                this->globalId,
                connectionStringPtr
            )
        );
    }

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

    auto lock = this->getRecursiveConfigLock();
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

    auto lock = this->getRecursiveConfigLock();

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
    const EventPacketPtr eventPacketFromConfig = Super::createDataDescriptorChangedEventPacket();

    DataDescriptorPtr valueDescriptorFromConfig;
    DataDescriptorPtr domainDescriptorFromConfig;
    std::tie(std::ignore, std::ignore, valueDescriptorFromConfig, domainDescriptorFromConfig) =
        parseDataDescriptorEventPacket(eventPacketFromConfig);

    std::scoped_lock lock(signalMutex);
    if (!mirroredDataDescriptor.assigned())
    {
        mirroredDataDescriptor = valueDescriptorFromConfig;
        if (!mirroredDomainDataDescriptor.assigned())
        {
            mirroredDomainDataDescriptor = domainDescriptorFromConfig;
            if (mirroredDomainSignal.assigned())
                mirroredDomainSignal.asPtr<IMirroredSignalPrivate>().setMirroredDataDescriptor(mirroredDomainDataDescriptor);
            else
            {
                const SignalPtr domain = this->onGetDomainSignal();
                if (domain.assigned())
                {
                    if (const auto mirroredDomain = domain.asPtrOrNull<IMirroredSignalPrivate>(); mirroredDomain.assigned())
                    {
                        mirroredDomain.setMirroredDataDescriptor(mirroredDomainDataDescriptor);
                    }
                }
            }
        }
    }

    return DataDescriptorChangedEventPacket(descriptorToEventPacketParam(mirroredDataDescriptor),
                                            descriptorToEventPacketParam(mirroredDomainDataDescriptor));
}

template <typename... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::subscribeInternal()
{
    auto activeStreamingSource = activeStreamingSourceRef.assigned() ? activeStreamingSourceRef.getRef() : nullptr;
    if (activeStreamingSource.assigned())
    {
        StringPtr signalRemoteId;
        ErrCode errCode = wrapHandlerReturn(this, &Self::onGetRemoteId, signalRemoteId);
        if (OPENDAQ_FAILED(errCode))
            return errCode;

        SignalPtr domainSignal;
        errCode = wrapHandlerReturn(this, &Self::onGetDomainSignal, domainSignal);
        if (OPENDAQ_FAILED(errCode))
            return errCode;

        StringPtr domainSignalRemoteId;
        if (domainSignal.assigned())
            domainSignalRemoteId = domainSignal.template asPtr<IMirroredSignalConfig>().getRemoteId();
        return activeStreamingSource.template asPtr<IStreamingPrivate>()->subscribeSignal(signalRemoteId, domainSignalRemoteId);
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
        StringPtr signalRemoteId;
        ErrCode errCode = wrapHandlerReturn(this, &Self::onGetRemoteId, signalRemoteId);
        if (OPENDAQ_FAILED(errCode))
            return errCode;

        SignalPtr domainSignal;
        errCode = wrapHandlerReturn(this, &Self::onGetDomainSignal, domainSignal);
        if (OPENDAQ_FAILED(errCode))
            return errCode;

        StringPtr domainSignalRemoteId;
        if (domainSignal.assigned())
            domainSignalRemoteId = domainSignal.template asPtr<IMirroredSignalConfig>().getRemoteId();
        return activeStreamingSource.template asPtr<IStreamingPrivate>()->unsubscribeSignal(signalRemoteId, domainSignalRemoteId);
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

    auto lock = this->getRecursiveConfigLock();
    *streamed = this->streamed;
    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode MirroredSignalBase<Interfaces...>::setStreamed(Bool streamed)
{
    auto lock = this->getRecursiveConfigLock();

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
ErrCode MirroredSignalBase<Interfaces...>::getSignalSerializeId(IString** serializeId)
{
    return this->getRemoteId(serializeId);
}

END_NAMESPACE_OPENDAQ
