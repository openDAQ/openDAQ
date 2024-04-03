/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <opendaq/context_ptr.h>
#include <coreobjects/property_object_ptr.h>
#include <opendaq/streaming.h>
#include <coretypes/impl.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>
#include <coreobjects/object_keys.h>
#include <opendaq/mirrored_signal_config_ptr.h>
#include <coretypes/validation.h>
#include <opendaq/streaming_private.h>
#include <opendaq/mirrored_signal_private_ptr.h>
#include <opendaq/ids_parser.h>
#include <opendaq/custom_log.h>
#include <opendaq/packet_factory.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename... Interfaces>
class StreamingImpl;

using Streaming = StreamingImpl<>;

template <typename... Interfaces>
class StreamingImpl : public ImplementationOfWeak<IStreaming, IStreamingPrivate, Interfaces...>
{
public:
    using Super = ImplementationOfWeak<IStreaming, IStreamingPrivate, Interfaces...>;
    using Self = StreamingImpl<Interfaces...>;

    explicit StreamingImpl(const StringPtr& connectionString, ContextPtr context);

    ~StreamingImpl() override;

    // IStreaming
    ErrCode INTERFACE_FUNC getActive(Bool* active) override;
    ErrCode INTERFACE_FUNC setActive(Bool active) override;
    ErrCode INTERFACE_FUNC addSignals(IList* signals) override;
    ErrCode INTERFACE_FUNC removeSignals(IList* signals) override;
    ErrCode INTERFACE_FUNC removeAllSignals() override;
    ErrCode INTERFACE_FUNC getConnectionString(IString** connectionString) const override;

    // IStreamingPrivate
    ErrCode INTERFACE_FUNC subscribeSignal(const StringPtr& signalRemoteId, const StringPtr& domainSignalRemoteId) override;
    ErrCode INTERFACE_FUNC unsubscribeSignal(const StringPtr& signalRemoteId, const StringPtr& domainSignalRemoteId) override;
    EventPacketPtr INTERFACE_FUNC createDataDescriptorChangedEventPacket(const StringPtr& signalRemoteId) override;
    ErrCode INTERFACE_FUNC detachRemovedSignal(const StringPtr& signalRemoteId) override;

protected:
    void addToAvailableSignals(const StringPtr& signalStreamingId);
    void removeFromAvailableSignals(const StringPtr& signalStreamingId);

    void startReconnection();
    void completeReconnection();

    /*!
     * @brief A function called when the active state of the Streaming is changed.
     * @param active The new active state of the Streaming.
     */
    virtual void onSetActive(bool active) = 0;

    /*!
     * @brief A function called when a signal is being added to the Streaming.
     * @param signal The signal to be added to the Streaming.
     */
    virtual void onAddSignal(const MirroredSignalConfigPtr& signal) = 0;

    /*!
     * @brief A function is called when signal is being removed from the Streaming.
     * @param signal The signal to be removed from the Streaming.
     */
    virtual void onRemoveSignal(const MirroredSignalConfigPtr& signal) = 0;

    /*!
     * @brief A function is called when signal is being subscribed within the Streaming.
     * @param signalStreamingId The streaming ID of the signal to be subscribed.
     */
    virtual void onSubscribeSignal(const StringPtr& signalStreamingId) = 0;

    /*!
     * @brief A function is called when signal is being unsubscribed within the Streaming.
     * @param signalStreamingId The streaming ID of the signal to be unsubscribed.
     */
    virtual void onUnsubscribeSignal(const StringPtr& signalStreamingId) = 0;

    /*!
     * @brief A function is invoked on creation of an initial DataDescriptor Changed Event Packet.
     * @param signalRemoteId The global remote ID of the signal for which the event is created.
     * @return The created DataDescriptor Changed Event Packet
     */
    virtual EventPacketPtr onCreateDataDescriptorChangedEventPacket(const StringPtr& signalStreamingId) = 0;

    void onPacket(const StringPtr& signalId, const PacketPtr& packet);
    void handleEventPacket(const MirroredSignalConfigPtr& signal, const EventPacketPtr& eventPacket);
    void triggerSubscribeAck(const StringPtr& signalStreamingId, bool subscribed);

    std::mutex sync;

    StringPtr connectionString;
    ContextPtr context;
    LoggerComponentPtr loggerComponent;

private:
    /*!
     * @brief A function used to get signal Id as it appears within the streaming.
     * @param signalRemoteId The signal remote Id.
     * @returns The signal ID used as a key to store/access the signal within the Streaming or
     * nullptr if signal is not available in Streaming
     */
    StringPtr getSignalStreamingId(const StringPtr& signalRemoteId);

    /*!
     * @brief Extracts signal item stored under `signalStreamingId` key from container and
     * inserts it under signal remote Id as a new key
     * @param signalStreamingId The signal Id as it appears within the streaming.
     */
    void remapUnavailableSignal(const StringPtr& signalStreamingId);

    /*!
     * @brief Extracts signal item stored under signal remote Id key from container and
     * inserts it under `signalStreamingId` as a new key. Sends subscribe request if signal subscribed.
     * @param signalStreamingId The signal Id as it appears within the streaming.
     */
    void remapAvailableSignal(const StringPtr& signalStreamingId);

    ErrCode removeStreamingSourceForAllSignals();
    void removeAllSignalsInternal();
    ErrCode doSubscribeSignal(const StringPtr& signalRemoteId);
    ErrCode doUnsubscribeSignal(const StringPtr& signalRemoteId);
    void resubscribeAvailableSignal(const StringPtr& signalStreamingId);

    bool isActive{false};
    bool isReconnecting{false};

    using SignalItem = std::pair<SizeT, WeakRefPtr<IMirroredSignalConfig>>;
    std::unordered_map<StringPtr, SignalItem, StringHash, StringEqualTo> streamingSignalsItems;

    std::unordered_set<StringPtr, StringHash, StringEqualTo> availableSignalIds;
    std::unordered_set<StringPtr, StringHash, StringEqualTo> availableSignalsIdsReconnection;
};

template <typename... Interfaces>
StreamingImpl<Interfaces...>::StreamingImpl(const StringPtr& connectionString, ContextPtr context)
    : connectionString(connectionString)
    , context(std::move(context))
    , loggerComponent(this->context.getLogger().getOrAddComponent(fmt::format("Streaming({})", connectionString)))
{
}

template <typename... Interfaces>
StreamingImpl<Interfaces...>::~StreamingImpl()
{
    try
    {
        ErrCode errCode = removeStreamingSourceForAllSignals();
        removeAllSignalsInternal();
        checkErrorInfo(errCode);
    }
    catch (const DaqException& e)
    {
        LOG_E("Failed to remove signals on streaming object destruction: {}", e.what());
    }
}

template <typename... Interfaces>
ErrCode StreamingImpl<Interfaces...>::getActive(Bool* active)
{
    OPENDAQ_PARAM_NOT_NULL(active);

    std::scoped_lock lock(sync);
    *active = this->isActive;

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode StreamingImpl<Interfaces...>::setActive(Bool active)
{
    if (static_cast<bool>(active) == this->isActive)
        return OPENDAQ_IGNORED;

    const ErrCode errCode = wrapHandler(this, &Self::onSetActive, active);
    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    std::scoped_lock lock(sync);
    this->isActive = active;

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode StreamingImpl<Interfaces...>::addSignals(IList* signals)
{
    OPENDAQ_PARAM_NOT_NULL(signals);

    const auto signalsPtr = ListPtr<ISignal>::Borrow(signals);
    for (const auto& signal : signalsPtr)
    {
        if (!signal.getPublic())
            continue;

        auto mirroredSignal = signal.asPtrOrNull<IMirroredSignalConfig>();
        if (mirroredSignal == nullptr)
        {
            return this->makeErrorInfo(OPENDAQ_ERR_NOINTERFACE,
                                       fmt::format(R"(Signal "{}" does not implement IMirroredSignalConfig interface.)",
                                                   signal.getGlobalId()));
        }

        auto signalRemoteId = mirroredSignal.getRemoteId();
        auto signalGlobalId = mirroredSignal.getGlobalId();
        {
            std::scoped_lock lock(sync);

            StringPtr signalIdKey = getSignalStreamingId(signalRemoteId);
            if (!signalIdKey.assigned())
            {
                LOG_W("Added signal with Ids (global /// remote) \"{}\" /// \"{}\" is not available in streaming yet",
                      signalGlobalId,
                      signalRemoteId);
                signalIdKey = signalRemoteId;
            }

            auto it = streamingSignalsItems.find(signalIdKey);

            if (it != streamingSignalsItems.end())
            {
                return this->makeErrorInfo(
                    OPENDAQ_ERR_DUPLICATEITEM,
                    fmt::format(
                        R"(Signal with Ids (global /// remote /// key) "{}" /// "{}" /// "{}" failed to add - signal already added to streaming "{}")",
                        signalGlobalId,
                        signalRemoteId,
                        signalIdKey,
                        connectionString
                    )
                );
            }

            ErrCode errCode = wrapHandler(this, &Self::onAddSignal, mirroredSignal);
            if (OPENDAQ_FAILED(errCode))
            {
                return errCode;
            }

            auto signalItem = std::make_pair(0, WeakRefPtr<IMirroredSignalConfig>(mirroredSignal));
            streamingSignalsItems.insert({signalIdKey, signalItem});
        }

        ErrCode errCode =
            daqTry([&]()
                   {
                       auto thisPtr = this->template borrowPtr<StreamingPtr>();
                       mirroredSignal.template asPtr<IMirroredSignalPrivate>().addStreamingSource(thisPtr);
                   });
        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }
    }

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode StreamingImpl<Interfaces...>::removeSignals(IList* signals)
{
    OPENDAQ_PARAM_NOT_NULL(signals);

    const auto signalsPtr = ListPtr<ISignal>::Borrow(signals);
    for (const auto& signal : signalsPtr)
    {
        auto mirroredSignalToRemove = signal.asPtrOrNull<IMirroredSignalConfig>();
        if (mirroredSignalToRemove == nullptr)
        {
            return this->makeErrorInfo(OPENDAQ_ERR_NOINTERFACE,
                                 fmt::format(R"(Signal "{}" does not implement IMirroredSignalConfig interface.)",
                                             signal.getGlobalId()));
        }

        ErrCode errCode =
            daqTry([&]()
                   {
                       mirroredSignalToRemove.template asPtr<IMirroredSignalPrivate>().removeStreamingSource(connectionString);
                   });
        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }

        auto signalRemoteId = mirroredSignalToRemove.getRemoteId();
        auto signalGlobalId = mirroredSignalToRemove.getGlobalId();
        {
            std::scoped_lock lock(sync);

            StringPtr signalIdKey = getSignalStreamingId(mirroredSignalToRemove.getRemoteId());
            if (!signalIdKey.assigned())
            {
                LOG_W("Removed signal with Ids (global /// remote) \"{}\" /// \"{}\" is not available in streaming",
                      signalGlobalId,
                      signalRemoteId);
                signalIdKey = signalRemoteId;
            }

            auto it = streamingSignalsItems.find(signalIdKey);
            if (it != streamingSignalsItems.end())
            {
                auto mirroredsignalRef = it->second.second;
                if (auto mirroredSignal = mirroredsignalRef.getRef(); mirroredSignal.assigned())
                {
                    onRemoveSignal(mirroredSignal);
                    streamingSignalsItems.erase(it);
                }
            }
            else
            {
                return this->makeErrorInfo(
                    OPENDAQ_ERR_NOTFOUND,
                    fmt::format(
                        R"(Signal with Ids (global /// remote /// key) "{}" /// "{}" /// "{}" failed to remove - signal not found in streaming "{}" )",
                        signalGlobalId,
                        signalRemoteId,
                        signalIdKey,
                        connectionString
                    )
                );
            }
        }
    }

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode StreamingImpl<Interfaces...>::removeAllSignals()
{
    ErrCode errCode = removeStreamingSourceForAllSignals();
    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    std::scoped_lock lock(sync);
    for (const auto& [_, signalItem] : streamingSignalsItems)
    {
        if (auto mirroredSignal = signalItem.second.getRef(); mirroredSignal.assigned())
        {
            errCode = wrapHandler(this, &Self::onRemoveSignal, mirroredSignal);
            if (OPENDAQ_FAILED(errCode))
            {
                return errCode;
            }
        }
    }

    removeAllSignalsInternal();

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode StreamingImpl<Interfaces...>::getConnectionString(IString** connectionString) const
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);

    *connectionString = this->connectionString.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode StreamingImpl<Interfaces...>::doSubscribeSignal(const StringPtr& signalRemoteId)
{
    std::scoped_lock lock(sync);

    bool skipSubscribeRequest = false;

    StringPtr signalId = getSignalStreamingId(signalRemoteId);
    if (!signalId.assigned())
    {
        LOG_E("Signal with remote Id \"{}\" is not yet available (will be subscribed when become available)",
              signalRemoteId);
        signalId = signalRemoteId;
        skipSubscribeRequest = true;
    }

    auto it = streamingSignalsItems.find(signalId);
    if (it != streamingSignalsItems.end())
    {
        if (it->second.first > 0)
            skipSubscribeRequest = true;
        it->second.first++;
    }
    else
    {
        return this->makeErrorInfo(
            OPENDAQ_ERR_NOTFOUND,
            fmt::format(
                R"(Signal with remote Id "{}" failed to subscribe - signal is not added to streaming "{}" )",
                signalRemoteId,
                connectionString
            )
        );
    }

    if (!skipSubscribeRequest)
    {
        ErrCode errCode = wrapHandler(this, &Self::onSubscribeSignal, signalId);
        if (OPENDAQ_FAILED(errCode))
            return errCode;
    }

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode StreamingImpl<Interfaces...>::subscribeSignal(const StringPtr& signalRemoteId, const StringPtr& domainSignalRemoteId)
{
    if (!signalRemoteId.assigned())
    {
        return this->makeErrorInfo(
            OPENDAQ_ERR_ARGUMENT_NULL,
            fmt::format(R"(Failed to subscribe - signal id is null)")
        );
    }

    if (signalRemoteId == domainSignalRemoteId)
    {
        return this->makeErrorInfo(
            OPENDAQ_ERR_INVALIDPARAMETER,
            fmt::format(
                R"(Signal "{}" failed to subscribe - provided domain signal Id is the same: "{}")",
                signalRemoteId,
                domainSignalRemoteId
            )
        );
    }

    if (domainSignalRemoteId.assigned())
    {
        ErrCode errCode = doSubscribeSignal(domainSignalRemoteId);
        if (OPENDAQ_FAILED(errCode))
            return errCode;
    }

    ErrCode errCode = doSubscribeSignal(signalRemoteId);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode StreamingImpl<Interfaces...>::doUnsubscribeSignal(const StringPtr& signalRemoteId)
{
    std::scoped_lock lock(sync);

    bool skipUnsubscribeRequest = false;

    StringPtr signalId = getSignalStreamingId(signalRemoteId);
    if (!signalId.assigned())
    {
        LOG_I("Signal with remote Id \"{}\" is not available", signalRemoteId);
        signalId = signalRemoteId;
        skipUnsubscribeRequest = true;
    }

    auto it = streamingSignalsItems.find(signalId);
    if (it != streamingSignalsItems.end())
    {
        if (it->second.first == 0)
        {
            return this->makeErrorInfo(
                OPENDAQ_ERR_INVALIDSTATE,
                fmt::format(
                    R"(Signal with remote Id "{}" failed to unsubscribe within streaming "{}", already unsubscribed)",
                    signalRemoteId,
                    connectionString
                )
            );
        }
        it->second.first--;
        if (it->second.first > 0)
            skipUnsubscribeRequest = true;
    }
    else
    {
        return this->makeErrorInfo(
            OPENDAQ_ERR_NOTFOUND,
            fmt::format(
                R"(Signal with remote Id "{}" failed to unsubscribe - signal is not added to streaming "{}" )",
                signalRemoteId,
                connectionString
            )
        );
    }

    if (!skipUnsubscribeRequest)
    {
        ErrCode errCode = wrapHandler(this, &Self::onUnsubscribeSignal, signalId);
        if (OPENDAQ_FAILED(errCode))
            return errCode;
    }

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode StreamingImpl<Interfaces...>::unsubscribeSignal(const StringPtr& signalRemoteId, const StringPtr& domainSignalRemoteId)
{
    if (!signalRemoteId.assigned())
    {
        return this->makeErrorInfo(
            OPENDAQ_ERR_ARGUMENT_NULL,
            fmt::format(R"(Failed to unsubscribe - signal id is null)")
        );
    }

    if (signalRemoteId == domainSignalRemoteId)
    {
        return this->makeErrorInfo(
            OPENDAQ_ERR_INVALIDPARAMETER,
            fmt::format(
                R"(Signal "{}" failed to unsubscribe - provided domain signal Id is the same: "{}")",
                signalRemoteId,
                domainSignalRemoteId
            )
        );
    }

    if (domainSignalRemoteId.assigned())
    {
        ErrCode errCode = doUnsubscribeSignal(domainSignalRemoteId);
        if (OPENDAQ_FAILED(errCode))
            return errCode;
    }

    ErrCode errCode = doUnsubscribeSignal(signalRemoteId);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
void StreamingImpl<Interfaces...>::resubscribeAvailableSignal(const StringPtr& signalStreamingId)
{
    if (const auto it = streamingSignalsItems.find(signalStreamingId); it != streamingSignalsItems.end())
    {
        auto signalSubscribersCount = it->second.first;
        auto signalRef = it->second.second;
        MirroredSignalConfigPtr signal = signalRef.getRef();

        if (signal.assigned() && signalSubscribersCount > 0)
        {
            onSubscribeSignal(signalStreamingId);
        }
    }
    // else - corresponding signal was not added, no actions required
}

template <typename... Interfaces>
EventPacketPtr StreamingImpl<Interfaces...>::createDataDescriptorChangedEventPacket(const StringPtr& signalRemoteId)
{
    std::scoped_lock lock(sync);

    StringPtr signalStreamingId = getSignalStreamingId(signalRemoteId);

    if (signalStreamingId.assigned())
    {
        return onCreateDataDescriptorChangedEventPacket(signalStreamingId);
    }
    else
    {
        LOG_E("Signal with remote id {} is not available", signalRemoteId);
        return DataDescriptorChangedEventPacket(nullptr, nullptr);
    }
}

template <typename... Interfaces>
ErrCode StreamingImpl<Interfaces...>::detachRemovedSignal(const StringPtr& signalRemoteId)
{
    std::scoped_lock lock(sync);

    StringPtr signalIdKey = getSignalStreamingId(signalRemoteId);
    if (!signalIdKey.assigned())
    {
        signalIdKey = signalRemoteId;
    }

    if (auto it = streamingSignalsItems.find(signalIdKey); it != streamingSignalsItems.end())
    {
        streamingSignalsItems.erase(it);
    }
    else
    {
        return this->makeErrorInfo(
            OPENDAQ_ERR_NOTFOUND,
            fmt::format(
                R"(Signal "{}" failed to remove - signal not found in streaming "{}" )",
                signalRemoteId,
                connectionString
            )
        );
    }

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode StreamingImpl<Interfaces...>::removeStreamingSourceForAllSignals()
{
    auto allSignals = List<IMirroredSignalConfig>();

    {
        std::scoped_lock lock(sync);
        for (const auto& [_, signalItem] : streamingSignalsItems)
        {
            if (auto mirroredSignal = signalItem.second.getRef(); mirroredSignal.assigned())
                allSignals.pushBack(mirroredSignal);
        }
    }

    for (const auto& signal : allSignals)
    {
        ErrCode errCode =
            daqTry([&]()
                   {
                       signal.template asPtr<IMirroredSignalPrivate>().removeStreamingSource(connectionString);
                   });
        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }
    }

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
void StreamingImpl<Interfaces...>::removeAllSignalsInternal()
{
    streamingSignalsItems.clear();
}

template <typename... Interfaces>
void StreamingImpl<Interfaces...>::onPacket(const StringPtr& signalId, const PacketPtr& packet)
{
    MirroredSignalConfigPtr signal;
    {
        std::scoped_lock lock(sync);

        if (!packet.assigned() || !this->isActive)
            return;

        if (auto it = streamingSignalsItems.find(signalId); it != streamingSignalsItems.end())
        {
            auto signalRef = it->second.second;
            signal = signalRef.getRef();
        }
    }
    if (signal.assigned() &&
        signal.getStreamed() &&
        signal.getActiveStreamingSource() == connectionString)
    {
        const auto eventPacket = packet.asPtrOrNull<IEventPacket>();
        if (eventPacket.assigned())
            handleEventPacket(signal, eventPacket);
        else
            signal.sendPacket(packet);
    }
}

template <typename... Interfaces>
void StreamingImpl<Interfaces...>::handleEventPacket(const MirroredSignalConfigPtr& signal, const EventPacketPtr& eventPacket)
{
    Bool forwardPacket = signal.template asPtr<IMirroredSignalPrivate>().triggerEvent(eventPacket);
    if (forwardPacket)
        signal.sendPacket(eventPacket);
}

template <typename... Interfaces>
void StreamingImpl<Interfaces...>::triggerSubscribeAck(const StringPtr& signalStreamingId, bool subscribed)
{
    MirroredSignalConfigPtr signal;
    {
        std::scoped_lock lock(sync);

        if (auto it = streamingSignalsItems.find(signalStreamingId); it != streamingSignalsItems.end())
        {
            auto signalRef = it->second.second;
            signal = signalRef.getRef();
        }
    }
    if (signal.assigned())
    {
        if (subscribed)
            signal.template asPtr<daq::IMirroredSignalPrivate>().subscribeCompleted(connectionString);
        else
            signal.template asPtr<daq::IMirroredSignalPrivate>().unsubscribeCompleted(connectionString);
    }
}

template <typename... Interfaces>
StringPtr StreamingImpl<Interfaces...>::getSignalStreamingId(const StringPtr& signalRemoteId)
{
    const auto it = std::find_if(
        this->availableSignalIds.begin(),
        this->availableSignalIds.end(),
        [&signalRemoteId](const StringPtr& signalStreamingId)
        {
            return IdsParser::idEndsWith(signalRemoteId.toStdString(), signalStreamingId.toStdString());
        }
    );

    if (it != this->availableSignalIds.end())
        return *it;
    else
        return nullptr;
}

template <typename... Interfaces>
void StreamingImpl<Interfaces...>::addToAvailableSignals(const StringPtr& signalStreamingId)
{
    std::scoped_lock lock(sync);

    if (isReconnecting)
    {
        if (const auto it = availableSignalsIdsReconnection.find(signalStreamingId); it == availableSignalsIdsReconnection.end())
        {
            availableSignalsIdsReconnection.insert(signalStreamingId);
        }
        else
        {
            LOG_E("Signal with id {} is already registered as available", signalStreamingId);
            throw DuplicateItemException("Signal with id {} is already registered as available in streaming {}",
                                         signalStreamingId,
                                         this->connectionString);
        }
    }
    else
    {
        if (const auto& it = availableSignalIds.find(signalStreamingId); it == availableSignalIds.end())
        {
            this->availableSignalIds.insert(signalStreamingId);
            remapAvailableSignal(signalStreamingId);
            resubscribeAvailableSignal(signalStreamingId);
        }
        else
        {
            LOG_E("Signal with id {} is already registered as available", signalStreamingId);
            throw DuplicateItemException("Signal with id {} is already registered as available in streaming {}",
                                         signalStreamingId,
                                         this->connectionString);
        }
    }
}

template <typename... Interfaces>
void StreamingImpl<Interfaces...>::removeFromAvailableSignals(const StringPtr& signalStreamingId)
{
    std::scoped_lock lock(sync);

    if (isReconnecting)
    {
        throw InvalidStateException("Signal unavailable command received during reconnection");
    }
    else
    {
        if (const auto& it = availableSignalIds.find(signalStreamingId); it != availableSignalIds.end())
        {
            this->availableSignalIds.erase(it);
            remapUnavailableSignal(signalStreamingId);
        }
        else
        {
            LOG_E("Signal with id {} was not registered as available", signalStreamingId);
            throw NotFoundException("Signal with id {} was not registered as available in streaming {}", signalStreamingId, this->connectionString);
        }
    }
}

template <typename... Interfaces>
void StreamingImpl<Interfaces...>::remapAvailableSignal(const StringPtr& signalStreamingId)
{
    // search for added signal with matching remote Id
    const auto it = std::find_if(
        this->streamingSignalsItems.begin(),
        this->streamingSignalsItems.end(),
        [&signalStreamingId](const std::pair<StringPtr, SignalItem>& item)
        {
            auto signalRemoteId = item.first;
            return IdsParser::idEndsWith(signalRemoteId.toStdString(), signalStreamingId.toStdString());
        }
    );

    if (it != streamingSignalsItems.end())
    {
        auto signalRemoteId = it->first;

        LOG_I("Added signal with Ids (remote /// streaming): {} /// {} became available",
              signalRemoteId,
              signalStreamingId);

        if (signalRemoteId != signalStreamingId)
        {
            auto item = streamingSignalsItems.extract(it);
            item.key() = signalStreamingId;
            streamingSignalsItems.insert(std::move(item));
        }
    }
    // else - corresponding signal was not added, no actions required
}

template <typename... Interfaces>
void StreamingImpl<Interfaces...>::remapUnavailableSignal(const StringPtr& signalStreamingId)
{
    if (auto it = streamingSignalsItems.find(signalStreamingId); it != streamingSignalsItems.end())
    {
        auto signalRef = it->second.second;
        MirroredSignalConfigPtr signal = signalRef.getRef();
        if (signal.assigned())
        {
            auto signalRemoteId = signal.getRemoteId();
            LOG_I("Added signal with Ids (remote /// streaming): {} /// {} became unavailable",
                  signalRemoteId,
                  signalStreamingId);
            if (signalRemoteId != signalStreamingId)
            {
                auto item = streamingSignalsItems.extract(it);
                item.key() = signalRemoteId;
                streamingSignalsItems.insert(std::move(item));
            }
        }
    }
    // else - corresponding signal was not added, no actions required
}

template <typename... Interfaces>
void StreamingImpl<Interfaces...>::startReconnection()
{
    std::scoped_lock lock(sync);

    availableSignalsIdsReconnection.clear();
    isReconnecting = true;
}

template <typename... Interfaces>
void StreamingImpl<Interfaces...>::completeReconnection()
{
    std::scoped_lock lock(sync);

    if (!isReconnecting)
        throw InvalidStateException("Fail to complete reconnection - reconnection was not started");

    for (const auto& signalId : availableSignalIds)
    {
        if (auto it = availableSignalsIdsReconnection.find(signalId); it == availableSignalsIdsReconnection.end())
        {
            // signal no longer available
            remapUnavailableSignal(signalId);
        }
    }

    for (const auto& signalId : availableSignalsIdsReconnection)
    {
        if (auto it = availableSignalIds.find(signalId); it == availableSignalIds.end())
        {
            // signal became available
            remapAvailableSignal(signalId);
        }
        resubscribeAvailableSignal(signalId);
    }

    availableSignalIds = availableSignalsIdsReconnection;
    availableSignalsIdsReconnection.clear();

    isReconnecting = false;
}

END_NAMESPACE_OPENDAQ
