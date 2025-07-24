/*
 * Copyright 2022-2025 openDAQ d.o.o.
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
#include <opendaq/input_port_impl.h>
#include <opendaq/mirrored_input_port_config_ptr.h>
#include <opendaq/streaming_to_device_ptr.h>
#include <opendaq/streaming_to_device_private.h>
#include <opendaq/mirrored_input_port_private_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename... Interfaces>
class MirroredInputPortBase;

using MirroredInputPort = MirroredInputPortBase<>;

template <typename... Interfaces>
class MirroredInputPortBase : public GenericInputPortImpl<IMirroredInputPortConfig, IMirroredInputPortPrivate, Interfaces...>
{
public:
    using Super = GenericInputPortImpl<IMirroredInputPortConfig, IMirroredInputPortPrivate, Interfaces...>;
    using Self = MirroredInputPortBase<Interfaces...>;

    explicit MirroredInputPortBase(const ContextPtr& context,
                                   const ComponentPtr& parent,
                                   const StringPtr& localId);

    virtual StringPtr onGetRemoteId() const = 0;

    // IMirroredInputPortConfig
    ErrCode INTERFACE_FUNC getRemoteId(IString** id) const override;
    ErrCode INTERFACE_FUNC getStreamingSources(IList** streamingConnectionStrings) override;
    ErrCode INTERFACE_FUNC setActiveStreamingSource(IString* streamingConnectionString) override;
    ErrCode INTERFACE_FUNC getActiveStreamingSource(IString** streamingConnectionString) override;
    ErrCode INTERFACE_FUNC deactivateStreaming() override;

    // IMirroredInputPortPrivate
    ErrCode INTERFACE_FUNC addStreamingSource(IStreamingToDevice* streaming) override;
    ErrCode INTERFACE_FUNC removeStreamingSource(IString* streamingConnectionString) override;
    ErrCode INTERFACE_FUNC getActiveStreamingSourceObject(IStreamingToDevice** streaming) override;
    ErrCode INTERFACE_FUNC getStreamingSourceObjects(IList** objects) override;

    // TODO
    // IInputPortConfig

//    ErrCode INTERFACE_FUNC setNotificationMethod(PacketReadyNotification method) override; // unavailable
//    ErrCode INTERFACE_FUNC getNotificationMethod(PacketReadyNotification* method) override; // unavailable
//    ErrCode INTERFACE_FUNC notifyPacketEnqueued(Bool queueWasEmpty) override; // unavailable
//    ErrCode INTERFACE_FUNC notifyPacketEnqueuedOnThisThread() override; // unavailable
//    ErrCode INTERFACE_FUNC setListener(IInputPortNotifications* port) override; // unavailable
//    ErrCode INTERFACE_FUNC getCustomData(IBaseObject** customData) override; // RPC
//    ErrCode INTERFACE_FUNC setCustomData(IBaseObject* customData) override; // unavailable
//    ErrCode INTERFACE_FUNC setRequiresSignal(Bool requiresSignal) override; // unavailable
//    ErrCode INTERFACE_FUNC getGapCheckingEnabled(Bool* gapCheckingEnabled) override; // RPC
//    ErrCode INTERFACE_FUNC notifyPacketEnqueuedWithScheduler() override; // unavailable

    // IInputPort
//    ErrCode INTERFACE_FUNC getConnection(IConnection** connection) override; // unavailable

protected:
    void removed() override;

private:
    // vector is used as the order of adding & accessing sources is important
    // store a pair string + weak reference to manage the removal of destroyed sources
    std::vector<std::pair<StringPtr, WeakRefPtr<IStreamingToDevice>>> streamingSourcesRefs;
    WeakRefPtr<IStreamingToDevice> activeStreamingSourceRef;
};

template <typename... Interfaces>
MirroredInputPortBase<Interfaces...>::MirroredInputPortBase(const ContextPtr& context,
                                                            const ComponentPtr& parent,
                                                            const StringPtr& localId)
    : Super(context, parent, localId, false)
    , activeStreamingSourceRef(nullptr)
{
}

template <typename... Interfaces>
ErrCode MirroredInputPortBase<Interfaces...>::getRemoteId(IString** id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    StringPtr signalRemoteId;
    const ErrCode errCode = wrapHandlerReturn(this, &Self::onGetRemoteId, signalRemoteId);

    *id = signalRemoteId.detach();

    return errCode;
}

template <typename... Interfaces>
void MirroredInputPortBase<Interfaces...>::removed()
{
    // TODO unsubscribe active streaming source
    activeStreamingSourceRef = nullptr;

    StringPtr inputPortRemoteId;
    ErrCode errCode = wrapHandlerReturn(this, &Self::onGetRemoteId, inputPortRemoteId);
    if (OPENDAQ_SUCCEEDED(errCode) && inputPortRemoteId.assigned())
    {
        for (const auto& [_, streamingRef] : streamingSourcesRefs)
        {
            if (auto streamingSource = streamingRef.getRef(); streamingSource.assigned())
                streamingSource.template asPtr<IStreamingToDevicePrivate>()->detachRemovedInputPort(inputPortRemoteId);
        }
        streamingSourcesRefs.clear();
    }
    Super::removed();
}

template <typename... Interfaces>
ErrCode MirroredInputPortBase<Interfaces...>::addStreamingSource(IStreamingToDevice* streaming)
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
        return DAQ_MAKE_ERROR_INFO(
            OPENDAQ_ERR_DUPLICATEITEM,
            fmt::format(
                R"(Input port with global Id "{}" already has streaming source "{}" )",
                this->globalId,
                connectionString
            )
        );
    }

    streamingSourcesRefs.push_back({connectionString, WeakRefPtr<IStreaming>(streamingPtr)});
    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode MirroredInputPortBase<Interfaces...>::removeStreamingSource(IString* streamingConnectionString)
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
        return DAQ_MAKE_ERROR_INFO(
            OPENDAQ_ERR_NOTFOUND,
            fmt::format(
                R"(Input port with global Id "{}" does not have streaming source "{}" )",
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
            // TODO unsubscribe active streaming source
            activeStreamingSourceRef = nullptr;
        }
    }

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode MirroredInputPortBase<Interfaces...>::getStreamingSources(IList** streamingConnectionStrings)
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
ErrCode MirroredInputPortBase<Interfaces...>::setActiveStreamingSource(IString* streamingConnectionString)
{
    OPENDAQ_PARAM_NOT_NULL(streamingConnectionString);

    const auto connectionStringPtr = StringPtr::Borrow(streamingConnectionString);
    auto thisPtr = this->template borrowPtr<MirroredInputPortConfigPtr>();

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
        return DAQ_MAKE_ERROR_INFO(
            OPENDAQ_ERR_NOTFOUND,
            fmt::format(
                R"(Input port with global Id "{}" does not have streaming source "{}" )",
                this->globalId,
                connectionStringPtr
            )
        );
    }

    StreamingPtr streamingSource = it->second.getRef();
    if (!streamingSource.assigned())
    {
        return DAQ_MAKE_ERROR_INFO(
            OPENDAQ_ERR_NOTFOUND,
            fmt::format(
                R"(Input port with global Id "{}": streaming source "{}" is already destroyed)",
                this->globalId,
                connectionStringPtr
            )
        );
    }

    // TODO unsubscribe old streaming source

    activeStreamingSourceRef = streamingSource;
    // TODO subscribe new streaming source

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode MirroredInputPortBase<Interfaces...>::getActiveStreamingSource(IString** streamingConnectionString)
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
ErrCode MirroredInputPortBase<Interfaces...>::deactivateStreaming()
{
    auto thisPtr = this->template borrowPtr<MirroredInputPortConfigPtr>();

    auto lock = this->getRecursiveConfigLock();

    ErrCode errCode = OPENDAQ_SUCCESS;
    // TODO unsubscribe streaming source
    activeStreamingSourceRef = nullptr;

    OPENDAQ_RETURN_IF_FAILED(errCode);

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode MirroredInputPortBase<Interfaces...>::getActiveStreamingSourceObject(IStreamingToDevice** streaming)
{
    OPENDAQ_PARAM_NOT_NULL(streaming);

    auto lock = this->getRecursiveConfigLock();
    auto activeStreamingSource = activeStreamingSourceRef.assigned() ? activeStreamingSourceRef.getRef() : nullptr;
    if (activeStreamingSource.assigned())
        *streaming = activeStreamingSource.addRefAndReturn();
    else
        *streaming = nullptr;

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode MirroredInputPortBase<Interfaces...>::getStreamingSourceObjects(IList** objects)
{
    OPENDAQ_PARAM_NOT_NULL(objects);

    auto objectsPtr = List<IStreamingToDevice>();

    auto lock = this->getRecursiveConfigLock();
    for (const auto& [connectionString, streamingRef] : streamingSourcesRefs)
    {
        auto streamingSource = streamingRef.getRef();
        if (streamingSource.assigned())
            objectsPtr.pushBack(streamingSource);
    }

    *objects = objectsPtr.detach();

    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
