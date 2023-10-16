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
#include <coreobjects/string_keys.h>
#include <opendaq/signal_remote_ptr.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename... Interfaces>
class StreamingImpl;

using Streaming = StreamingImpl<>;

template <typename... Interfaces>
class StreamingImpl : public ImplementationOf<IStreaming, Interfaces...>
{
public:
    using Super = ImplementationOf<IStreaming, Interfaces...>;
    using Self = StreamingImpl<Interfaces...>;

    explicit StreamingImpl(const StringPtr& connectionString,
                           ContextPtr context);

    ~StreamingImpl() override;

    ErrCode INTERFACE_FUNC getActive(Bool* active) override;
    ErrCode INTERFACE_FUNC setActive(Bool active) override;
    ErrCode INTERFACE_FUNC addSignals(IList* signals) override;
    ErrCode INTERFACE_FUNC removeSignals(IList* signals) override;
    ErrCode INTERFACE_FUNC removeAllSignals() override;
    ErrCode INTERFACE_FUNC getConnectionString(IString** connectionString) const override;

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
    virtual StringPtr onAddSignal(const SignalRemotePtr& signal) = 0;

    /*!
     * @brief A function is called when signal is being removed from the Streaming.
     * @param signal The signal to be removed from the Streaming.
     */
    virtual void onRemoveSignal(const SignalRemotePtr& signal) = 0;

    void removeAllSignalsInternal();

    std::mutex sync;
    StringPtr connectionString;
    ContextPtr context;
    bool isActive{};
    std::unordered_map<StringPtr, SignalRemotePtr, StringHash, StringEqualTo> streamingSignals;
};

template <typename... Interfaces>
StreamingImpl<Interfaces...>::StreamingImpl(const StringPtr &connectionString, ContextPtr context)
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
        auto remoteSignal = signal.asPtrOrNull<ISignalRemote>();
        if (remoteSignal == nullptr)
        {
            return this->makeErrorInfo(OPENDAQ_ERR_NOINTERFACE,
                                       fmt::format(R"(Signal "{}" does not implement ISignalRemote interface.)",
                                                   signal.getGlobalId()));
        }

        auto it = std::find_if(
            streamingSignals.begin(),
            streamingSignals.end(),
            [remoteSignal](const std::pair<StringPtr, SignalRemotePtr>& element) -> bool
            {
                return element.second == remoteSignal;
            }
        );

        if (it != streamingSignals.end())
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
        ErrCode errCode = wrapHandlerReturn(this, &Self::onAddSignal, streamingSignalId, remoteSignal);
        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }

        if (streamingSignals.find(streamingSignalId) != streamingSignals.end())
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
        streamingSignals.insert({streamingSignalId, remoteSignal});

        auto thisPtr = this->template borrowPtr<StreamingPtr>();
        remoteSignal.addStreamingSource(thisPtr);
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
        auto remoteSignal = signal.asPtrOrNull<ISignalRemote>();
        if (remoteSignal == nullptr)
        {
            return this->makeErrorInfo(OPENDAQ_ERR_NOINTERFACE,
                                 fmt::format(R"(Signal "{}" does not implement ISignalRemote interface.)",
                                             signal.getGlobalId()));
        }

        auto it = std::find_if(
            streamingSignals.begin(),
            streamingSignals.end(),
            [remoteSignal](const std::pair<StringPtr, SignalRemotePtr>& element) -> bool
            {
                return element.second == remoteSignal;
            }
            );
        if (it != streamingSignals.end())
        {
            ErrCode errCode = wrapHandler(this, &Self::onRemoveSignal, remoteSignal);
            if (OPENDAQ_FAILED(errCode))
            {
                return errCode;
            }
            streamingSignals.erase(it);

            auto thisPtr = this->template borrowPtr<StreamingPtr>();
            remoteSignal.removeStreamingSource(thisPtr);
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

    for (const auto& element : streamingSignals)
    {
        auto remoteSignal = element.second;
        ErrCode errCode = wrapHandler(this, &Self::onRemoveSignal, remoteSignal);
        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
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
void StreamingImpl<Interfaces...>::removeAllSignalsInternal()
{
    for (const auto& element : streamingSignals)
    {
        auto remoteSignal = element.second;
        auto thisPtr = this->template borrowPtr<StreamingPtr>();
        remoteSignal.removeStreamingSource(thisPtr);
    }
    streamingSignals.clear();
}

END_NAMESPACE_OPENDAQ
