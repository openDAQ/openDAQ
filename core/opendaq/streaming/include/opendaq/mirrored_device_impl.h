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

#include <opendaq/mirrored_device_config.h>
#include <opendaq/device_impl.h>
#include <opendaq/streaming_ptr.h>
#include <opendaq/streaming_private.h>

BEGIN_NAMESPACE_OPENDAQ

template <typename... Interfaces>
class MirroredDeviceBase;

using MirroredDevice = MirroredDeviceBase<>;

template <typename... Interfaces>
class MirroredDeviceBase : public DeviceBase<IMirroredDeviceConfig, Interfaces...>
{
public:
    using Super = DeviceBase<IMirroredDeviceConfig, Interfaces...>;
    using Self = MirroredDeviceBase<Interfaces...>;

    explicit MirroredDeviceBase(const ContextPtr& ctx,
                                const ComponentPtr& parent,
                                const StringPtr& localId,
                                const StringPtr& className = nullptr,
                                const StringPtr& name = nullptr);

    // IMirroredDevice
    ErrCode INTERFACE_FUNC getStreamingSources(IList** streamingSources) override;

    // IMirroredDeviceConfig
    ErrCode INTERFACE_FUNC addStreamingSource(IStreaming* streamingSource) override;
    ErrCode INTERFACE_FUNC removeStreamingSource(IString* streamingConnectionString) override;

protected:
    void removed() override;

    StreamingPtr onAddStreaming(const StringPtr& connectionString, const PropertyObjectPtr& config) override;

private:
    std::vector<StreamingPtr> streamingSources;
};

template <typename... Interfaces>
MirroredDeviceBase<Interfaces...>::MirroredDeviceBase(const ContextPtr& ctx,
                                                      const ComponentPtr& parent,
                                                      const StringPtr& localId,
                                                      const StringPtr& className,
                                                      const StringPtr& name)
    : Super(ctx, parent, localId, className, name)
{
}

template <typename... Interfaces>
ErrCode MirroredDeviceBase<Interfaces...>::getStreamingSources(IList** streamingSources)
{
    OPENDAQ_PARAM_NOT_NULL(streamingSources);

    auto streamingSourcesPtr = List<IStreaming>();

    auto lock = this->getRecursiveConfigLock();
    for (const auto& streaming : this->streamingSources)
    {
        streamingSourcesPtr.pushBack(streaming);
    }

    *streamingSources = streamingSourcesPtr.detach();
    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode MirroredDeviceBase<Interfaces...>::addStreamingSource(IStreaming* streamingSource)
{
    OPENDAQ_PARAM_NOT_NULL(streamingSource);

    const auto streamingPtr = StreamingPtr::Borrow(streamingSource);
    const auto connectionString = streamingPtr.getConnectionString();

    auto lock = this->getRecursiveConfigLock();

    auto it = std::find_if(streamingSources.begin(),
                           streamingSources.end(),
                           [&connectionString](const StreamingPtr& item)
                           {
                               return connectionString == item.getConnectionString();
                           });

    if (it != streamingSources.end())
    {
        return this->makeErrorInfo(
            OPENDAQ_ERR_DUPLICATEITEM,
            fmt::format(
                R"(Device with global Id "{}" already has streaming source "{}" )",
                this->globalId,
                connectionString
            )
        );
    }

    streamingSources.push_back(streamingPtr);

    auto errCode =
        daqTry(
            [this, streamingPtr] {
                this->connectionStatusContainer.addStreamingConnectionStatus(
                    streamingPtr.getConnectionString(),
                    streamingPtr.getConnectionStatus(),
                    streamingPtr
                );
            }
        );
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    const auto thisPtr = this->template borrowPtr<DevicePtr>();
    errCode = streamingPtr.template asPtr<IStreamingPrivate>()->setOwnerDevice(thisPtr);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode MirroredDeviceBase<Interfaces...>::removeStreamingSource(IString* streamingConnectionString)
{
    OPENDAQ_PARAM_NOT_NULL(streamingConnectionString);

    auto lock = this->getRecursiveConfigLock();

    const auto streamingConnectionStringPtr = StringPtr::Borrow(streamingConnectionString);

    auto it = std::find_if(streamingSources.begin(),
                           streamingSources.end(),
                           [&streamingConnectionStringPtr](const StreamingPtr& item)
                           {
                               return streamingConnectionStringPtr == item.getConnectionString();
                           });

    if (it == streamingSources.end())
    {
        return this->makeErrorInfo(
            OPENDAQ_ERR_NOTFOUND,
            fmt::format(
                R"(Device with global Id "{}" does not have streaming source "{}" )",
                this->globalId,
                streamingConnectionStringPtr
            )
        );
    }

    auto errCode =
        daqTry(
            [this, streamingConnectionStringPtr] {
                this->connectionStatusContainer.removeStreamingConnectionStatus(streamingConnectionStringPtr);
            }
        );
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    errCode = (*it).template asPtr<IStreamingPrivate>()->setOwnerDevice(nullptr);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    streamingSources.erase(it);

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
void MirroredDeviceBase<Interfaces...>::removed()
{
    for (const auto& streamingSource : streamingSources)
    {
        this->connectionStatusContainer.removeStreamingConnectionStatus(streamingSource.getConnectionString());
        checkErrorInfo(streamingSource.template asPtr<IStreamingPrivate>()->setOwnerDevice(nullptr));
    }

    // disconnects all streaming connections
    streamingSources.clear();

    Super::removed();
}

template <typename... Interfaces>
StreamingPtr MirroredDeviceBase<Interfaces...>::onAddStreaming(const StringPtr& connectionString, const PropertyObjectPtr& config)
{
    auto lock = this->getRecursiveConfigLock();

    auto it = std::find_if(streamingSources.begin(),
                           streamingSources.end(),
                           [&connectionString](const StreamingPtr& item)
                           {
                               return connectionString == item.getConnectionString();
                           });

    if (it != streamingSources.end())
    {
        throw DuplicateItemException(
            fmt::format(
                R"(Device with global Id "{}" already has streaming source "{}" )",
                this->globalId,
                connectionString
            )
        );
    }

    const ModuleManagerUtilsPtr managerUtils = this->context.getModuleManager().template asPtr<IModuleManagerUtils>();
    auto streamingPtr = managerUtils.createStreaming(connectionString, config);
    streamingSources.push_back(streamingPtr);

    this->connectionStatusContainer.addStreamingConnectionStatus(streamingPtr.getConnectionString(),
                                                                 streamingPtr.getConnectionStatus(),
                                                                 streamingPtr);
    const auto thisPtr = this->template borrowPtr<DevicePtr>();
    checkErrorInfo(streamingPtr.template asPtr<IStreamingPrivate>()->setOwnerDevice(thisPtr));

    return streamingPtr;
}

END_NAMESPACE_OPENDAQ
