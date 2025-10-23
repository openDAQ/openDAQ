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

#include <opendaq/mirrored_device_config_ptr.h>
#include <opendaq/device_impl.h>
#include <opendaq/streaming_ptr.h>
#include <opendaq/streaming_private.h>
#include <opendaq/streaming_source_manager.h>

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
    ~MirroredDeviceBase() override;

    // IMirroredDevice
    ErrCode INTERFACE_FUNC getStreamingSources(IList** streamingSources) override;
    ErrCode INTERFACE_FUNC getRemoteId(IString** id) const override;

    // IMirroredDeviceConfig
    ErrCode INTERFACE_FUNC addStreamingSource(IStreaming* streamingSource) override;
    ErrCode INTERFACE_FUNC removeStreamingSource(IString* streamingConnectionString) override;
    ErrCode INTERFACE_FUNC serializeForUpdateLocally(ISerializer* serializer) override;

    // IComponentPrivate
    ErrCode INTERFACE_FUNC setComponentConfig(IPropertyObject* config) override;

protected:
    void removed() override;

    StreamingPtr onAddStreaming(const StringPtr& connectionString, const PropertyObjectPtr& config) override;

    virtual bool isAddedToLocalComponentTree();
    virtual StringPtr onGetRemoteId() const = 0;

private:
    std::vector<StreamingPtr> streamingSources;
    StreamingSourceManagerPtr streamingSourceManager;
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
MirroredDeviceBase<Interfaces...>::~MirroredDeviceBase()
{
    streamingSourceManager.reset();
}

template <typename... Interfaces>
ErrCode MirroredDeviceBase<Interfaces...>::getStreamingSources(IList** streamingSources)
{
    OPENDAQ_PARAM_NOT_NULL(streamingSources);

    auto streamingSourcesPtr = List<IStreaming>();

    auto lock = this->getRecursiveConfigLock2();
    for (const auto& streaming : this->streamingSources)
    {
        streamingSourcesPtr.pushBack(streaming);
    }

    *streamingSources = streamingSourcesPtr.detach();
    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode MirroredDeviceBase<Interfaces...>::getRemoteId(IString** id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    StringPtr signalRemoteId;
    const ErrCode errCode = wrapHandlerReturn(this, &Self::onGetRemoteId, signalRemoteId);

    *id = signalRemoteId.detach();

    return errCode;
}

template <typename... Interfaces>
ErrCode MirroredDeviceBase<Interfaces...>::addStreamingSource(IStreaming* streamingSource)
{
    OPENDAQ_PARAM_NOT_NULL(streamingSource);

    const auto streamingPtr = StreamingPtr::Borrow(streamingSource);
    const auto connectionString = streamingPtr.getConnectionString();

    auto lock = this->getRecursiveConfigLock2();

    auto it = std::find_if(streamingSources.begin(),
                           streamingSources.end(),
                           [&connectionString](const StreamingPtr& item)
                           {
                               return connectionString == item.getConnectionString();
                           });

    if (it != streamingSources.end())
    {
        return DAQ_MAKE_ERROR_INFO(
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
    OPENDAQ_RETURN_IF_FAILED(errCode);

    const auto thisPtr = this->template borrowPtr<DevicePtr>();
    errCode = streamingPtr.template asPtr<IStreamingPrivate>()->setOwnerDevice(thisPtr);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode MirroredDeviceBase<Interfaces...>::removeStreamingSource(IString* streamingConnectionString)
{
    OPENDAQ_PARAM_NOT_NULL(streamingConnectionString);

    auto lock = this->getRecursiveConfigLock2();

    const auto streamingConnectionStringPtr = StringPtr::Borrow(streamingConnectionString);

    auto it = std::find_if(streamingSources.begin(),
                           streamingSources.end(),
                           [&streamingConnectionStringPtr](const StreamingPtr& item)
                           {
                               return streamingConnectionStringPtr == item.getConnectionString();
                           });

    if (it == streamingSources.end())
    {
        return DAQ_MAKE_ERROR_INFO(
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
    OPENDAQ_RETURN_IF_FAILED(errCode);

    errCode = (*it).template asPtr<IStreamingPrivate>()->setOwnerDevice(nullptr);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    streamingSources.erase(it);

    return OPENDAQ_SUCCESS;
}

template <typename... Interfaces>
ErrCode MirroredDeviceBase<Interfaces...>::serializeForUpdateLocally(ISerializer* serializer)
{
    return Super::serializeForUpdate(serializer);
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

    streamingSourceManager.reset();

    Super::removed();
}

template <typename... Interfaces>
ErrCode MirroredDeviceBase<Interfaces...>::setComponentConfig(IPropertyObject* config)
{
    ErrCode errCode = Super::setComponentConfig(config);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    if (this->componentConfig.assigned() && this->componentConfig.hasProperty("General"))
    {
        auto deviceSelf = this->template borrowPtr<DevicePtr>();
        PropertyObjectPtr generalConfig = this->componentConfig.getPropertyValue("General");
        bool automaticallyConnectStreamings = generalConfig.getPropertyValue("AutomaticallyConnectStreaming");
        if (automaticallyConnectStreamings &&
            !(generalConfig.getPropertyValue("StreamingConnectionHeuristic") == 2)) // is not "NotConnected"
            streamingSourceManager = std::make_shared<StreamingSourceManager>(this->context, deviceSelf, this->componentConfig);
    }

    return errCode;
}

template <typename... Interfaces>
StreamingPtr MirroredDeviceBase<Interfaces...>::onAddStreaming(const StringPtr& connectionString, const PropertyObjectPtr& config)
{
    auto lock = this->getRecursiveConfigLock2();

    auto it = std::find_if(streamingSources.begin(),
                           streamingSources.end(),
                           [&connectionString](const StreamingPtr& item)
                           {
                               return connectionString == item.getConnectionString();
                           });

    if (it != streamingSources.end())
    {
        DAQ_THROW_EXCEPTION(DuplicateItemException,
                            R"(Device with global Id "{}" already has streaming source "{}" )",
                            this->globalId,
                            connectionString
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

template <typename... Interfaces>
bool MirroredDeviceBase<Interfaces...>::isAddedToLocalComponentTree()
{
    return false;
}

END_NAMESPACE_OPENDAQ
