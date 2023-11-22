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
#include <opendaq/mirrored_signal_private.h>

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
    ErrCode INTERFACE_FUNC subscribeSignal(const MirroredSignalConfigPtr& signal) override;
    ErrCode INTERFACE_FUNC unsubscribeSignal(const MirroredSignalConfigPtr& signal) override;
    EventPacketPtr INTERFACE_FUNC createDataDescriptorChangedEventPacket(const MirroredSignalConfigPtr& signal) override;

protected:
    /*!
     * @brief A function called when the active state of the Streaming is changed.
     * @param active The new active state of the Streaming.
     */
    virtual void onSetActive(bool active) = 0;

    /*!
     * @brief A function called when a signal is being added to the Streaming.
     * @param signal The signal to be added to the Streaming.
     * @returns The signal ID used as a key to store/access the signal within the Streaming.
     */
    virtual StringPtr onAddSignal(const MirroredSignalConfigPtr& signal) = 0;

    /*!
     * @brief A function is called when signal is being removed from the Streaming.
     * @param signal The signal to be removed from the Streaming.
     */
    virtual void onRemoveSignal(const MirroredSignalConfigPtr& signal) = 0;

    /*!
     * @brief A function is called when signal is being subscribed within the Streaming.
     * @param signal The signal to be subscribed within the Streaming.
     */
    virtual void onSubscribeSignal(const MirroredSignalConfigPtr& signal) = 0;

    /*!
     * @brief A function is called when signal is being unsubscribed within the Streaming.
     * @param signal The signal to be unsubscribed within the Streaming.
     */
    virtual void onUnsubscribeSignal(const MirroredSignalConfigPtr& signal) = 0;

    virtual EventPacketPtr onCreateDataDescriptorChangedEventPacket(const MirroredSignalConfigPtr& signal) = 0;

    void removeAllSignalsInternal();

    std::mutex sync;
    StringPtr connectionString;
    ContextPtr context;
    bool isActive{};
    std::unordered_map<StringPtr, WeakRefPtr<IMirroredSignalConfig>, StringHash, StringEqualTo> streamingSignalsRefs;
};

template <typename... Interfaces>
StreamingImpl<Interfaces...>::StreamingImpl(const StringPtr& connectionString, ContextPtr context)
    : connectionString(connectionString)
    , context(std::move(context))
{
}

template <typename... Interfaces>
StreamingImpl<Interfaces...>::~StreamingImpl()
{
    removeAllSignalsInternal();
}

template <typename... Interfaces>
ErrCode StreamingImpl<Interfaces...>::getActive(Bool *active)
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
ErrCode StreamingImpl<Interfaces...>::addSignals(IList *signals)
{
    OPENDAQ_PARAM_NOT_NULL(signals);

    std::scoped_lock lock(sync);

    const auto signalsPtr = ListPtr<ISignal>::Borrow(signals);
    for (const auto& signal : signalsPtr)
    {
        auto mirroredSignal = signal.asPtrOrNull<IMirroredSignalConfig>();
        if (mirroredSignal == nullptr)
        {
            return this->makeErrorInfo(OPENDAQ_ERR_NOINTERFACE,
                                       fmt::format(R"(Signal "{}" does not implement IMirroredSignalConfig interface.)",
                                                   signal.getGlobalId()));
        }

        auto it = std::find_if(
            streamingSignalsRefs.begin(),
            streamingSignalsRefs.end(),
            [mirroredSignal](const std::pair<StringPtr, WeakRefPtr<IMirroredSignalConfig>>& element) -> bool
            {
                auto signalRef = element.second;
                if (signalRef.assigned())
                    return signalRef.getRef() == mirroredSignal;
                else
                    return false;
            }
        );

        if (it != streamingSignalsRefs.end())
        {
            return this->makeErrorInfo(
                OPENDAQ_ERR_DUPLICATEITEM,
                fmt::format(
                    R"(Signal "{}" failed to add - signal already added to streaming "{}")",
                    signal.getGlobalId(),
                    connectionString
                    )
                );
        }

        StringPtr streamingSignalId;
        ErrCode errCode = wrapHandlerReturn(this, &Self::onAddSignal, streamingSignalId, mirroredSignal);
        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }

        if (streamingSignalsRefs.find(streamingSignalId) != streamingSignalsRefs.end())
        {
            return this->makeErrorInfo(
                OPENDAQ_ERR_DUPLICATEITEM,
                fmt::format(
                    R"(Signal "{}" failed to add - key Id "{}" duplicates existed in streaming "{}")",
                    signal.getGlobalId(),
                    streamingSignalId,
                    connectionString
                    )
                );
        }
        streamingSignalsRefs.insert({streamingSignalId, WeakRefPtr<IMirroredSignalConfig>(mirroredSignal)});

        auto thisPtr = this->template borrowPtr<StreamingPtr>();
        errCode = mirroredSignal.template asPtr<IMirroredSignalPrivate>()->addStreamingSource(thisPtr);
        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }
    }

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode StreamingImpl<Interfaces...>::removeSignals(IList *signals)
{
    OPENDAQ_PARAM_NOT_NULL(signals);

    std::scoped_lock lock(sync);

    const auto signalsPtr = ListPtr<ISignal>::Borrow(signals);
    for (const auto& signal : signalsPtr)
    {
        auto mirroredSignal = signal.asPtrOrNull<IMirroredSignalConfig>();
        if (mirroredSignal == nullptr)
        {
            return this->makeErrorInfo(OPENDAQ_ERR_NOINTERFACE,
                                 fmt::format(R"(Signal "{}" does not implement IMirroredSignalConfig interface.)",
                                             signal.getGlobalId()));
        }

        auto it = std::find_if(
            streamingSignalsRefs.begin(),
            streamingSignalsRefs.end(),
            [mirroredSignal](const std::pair<StringPtr, WeakRefPtr<IMirroredSignalConfig>>& element) -> bool
            {
                auto signalRef = element.second;
                if (signalRef.assigned())
                    return signalRef.getRef() == mirroredSignal;
                else
                    return false;
            }
        );
        if (it != streamingSignalsRefs.end())
        {
            ErrCode errCode = wrapHandler(this, &Self::onRemoveSignal, mirroredSignal);
            if (OPENDAQ_FAILED(errCode))
            {
                return errCode;
            }
            streamingSignalsRefs.erase(it);

            errCode = mirroredSignal.template asPtr<IMirroredSignalPrivate>()->removeStreamingSource(connectionString);
            if (OPENDAQ_FAILED(errCode))
            {
                return errCode;
            }
        }
        else
        {
            return this->makeErrorInfo(
                OPENDAQ_ERR_NOTFOUND,
                fmt::format(
                    R"(Signal "{}" failed to remove - signal not found in streaming "{}" )",
                    signal.getGlobalId(),
                    connectionString
                    )
                );
        }
    }

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode StreamingImpl<Interfaces...>::removeAllSignals()
{
    std::scoped_lock lock(sync);

    for (const auto& [_, mirroredSignalRef] : streamingSignalsRefs)
    {
        if (mirroredSignalRef.assigned())
        {
            auto mirroredSignal = mirroredSignalRef.getRef();
            if (mirroredSignal.assigned())
            {
                ErrCode errCode = wrapHandler(this, &Self::onRemoveSignal, mirroredSignal);
                if (OPENDAQ_FAILED(errCode))
                {
                    return errCode;
                }
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
ErrCode StreamingImpl<Interfaces...>::subscribeSignal(const MirroredSignalConfigPtr& signal)
{
    ErrCode errCode = wrapHandler(this, &Self::onSubscribeSignal, signal);
    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode StreamingImpl<Interfaces...>::unsubscribeSignal(const MirroredSignalConfigPtr& signal)
{
    ErrCode errCode = wrapHandler(this, &Self::onUnsubscribeSignal, signal);
    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
EventPacketPtr StreamingImpl<Interfaces...>::createDataDescriptorChangedEventPacket(const MirroredSignalConfigPtr& signal)
{
    return onCreateDataDescriptorChangedEventPacket(signal);
}

template <typename... Interfaces>
void StreamingImpl<Interfaces...>::removeAllSignalsInternal()
{
    for (const auto& [_, mirroredSignalRef] : streamingSignalsRefs)
    {
        if (mirroredSignalRef.assigned())
        {
            auto mirroredSignal = mirroredSignalRef.getRef();
            if (mirroredSignal.assigned())
                mirroredSignal.template asPtr<IMirroredSignalPrivate>()->removeStreamingSource(connectionString);
        }
    }
    streamingSignalsRefs.clear();
}

END_NAMESPACE_OPENDAQ
