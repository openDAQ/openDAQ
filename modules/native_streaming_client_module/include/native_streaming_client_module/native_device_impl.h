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

#include <native_streaming_client_module/common.h>

#include <config_protocol/config_protocol_client.h>
#include <config_protocol/config_client_device_impl.h>

#include <native_streaming_protocol/native_streaming_client_handler.h>

#include <opendaq/context_ptr.h>
#include <opendaq/streaming_ptr.h>

#include <future>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

static const char* NativeConfigurationDeviceTypeId = "OpenDAQNativeConfiguration";
static const char* NativeStreamingTypeId = "OpenDAQNativeStreaming";
static const char* NativeConfigurationDevicePrefix = "daq.nd";

class NativeDeviceImpl;

class NativeDeviceHelper : public std::enable_shared_from_this<NativeDeviceHelper>
{
public:
    explicit NativeDeviceHelper(const ContextPtr& context,
                                opendaq_native_streaming_protocol::NativeStreamingClientHandlerPtr transportProtocolClient,
                                SizeT configProtocolRequestTimeout,
                                std::shared_ptr<boost::asio::io_context> processingIOContextPtr,
                                std::shared_ptr<boost::asio::io_context> reconnectionProcessingIOContextPtr,
                                std::thread::id reconnectionProcessingThreadId);
    ~NativeDeviceHelper();

    void setupProtocolClients(const ContextPtr& context);
    DevicePtr connectAndGetDevice(const ComponentPtr& parent, uint16_t protocolVersion);

    void subscribeToCoreEvent(const ContextPtr& context);
    void unsubscribeFromCoreEvent(const ContextPtr& context);

    void closeConnectionOnRemoval();

private:
    void connectionStatusChangedHandler(opendaq_native_streaming_protocol::ClientConnectionStatus status);
    config_protocol::PacketBuffer doConfigRequest(const config_protocol::PacketBuffer& reqPacket);
    std::future<config_protocol::PacketBuffer> registerConfigRequest(uint64_t requestId);
    void unregisterConfigRequest(uint64_t requestId);
    void cancelPendingConfigRequests(const DaqException& e);
    void processConfigPacket(config_protocol::PacketBuffer&& packet);
    void coreEventCallback(ComponentPtr& sender, CoreEventArgsPtr& eventArgs);
    void componentAdded(const ComponentPtr& sender, const CoreEventArgsPtr& eventArgs);
    void componentUpdated(const ComponentPtr& sender, const CoreEventArgsPtr& eventArgs);
    void enableStreamingForComponent(const ComponentPtr& component);
    void tryAddSignalToStreaming(const SignalPtr& signal, const StreamingPtr& streaming);
    void setSignalActiveStreamingSource(const SignalPtr& signal, const StreamingPtr& streaming);

    std::shared_ptr<boost::asio::io_context> processingIOContextPtr;
    std::shared_ptr<boost::asio::io_context> reconnectionProcessingIOContextPtr;
    std::thread::id reconnectionProcessingThreadId;

    LoggerComponentPtr loggerComponent;
    std::unique_ptr<config_protocol::ConfigProtocolClient<NativeDeviceImpl>> configProtocolClient;
    opendaq_native_streaming_protocol::NativeStreamingClientHandlerPtr transportClientHandler;
    std::unordered_map<size_t, std::promise<config_protocol::PacketBuffer>> replyPackets;
    WeakRefPtr<IDevice> deviceRef;
    opendaq_native_streaming_protocol::ClientConnectionStatus connectionStatus;
    std::chrono::milliseconds configProtocolRequestTimeout;
    std::mutex sync;
};

DECLARE_OPENDAQ_INTERFACE(INativeDevicePrivate, IBaseObject)
{
    virtual void INTERFACE_FUNC attachDeviceHelper(std::shared_ptr<NativeDeviceHelper> deviceHelper) = 0;
    virtual void INTERFACE_FUNC updateDeviceInfo(const StringPtr& connectionString) = 0;
    virtual void INTERFACE_FUNC publishConnectionStatus(ConstCharPtr statusValue) = 0;
};

class NativeDeviceImpl final : public config_protocol::GenericConfigClientDeviceImpl<config_protocol::ConfigClientDeviceBase<INativeDevicePrivate>>
{
public:
    using Super = config_protocol::GenericConfigClientDeviceImpl<config_protocol::ConfigClientDeviceBase<INativeDevicePrivate>>;

    explicit NativeDeviceImpl(const config_protocol::ConfigProtocolClientCommPtr& configProtocolClientComm,
                              const std::string& remoteGlobalId,
                              const ContextPtr& ctx,
                              const ComponentPtr& parent,
                              const StringPtr& localId,
                              const StringPtr& className = nullptr);
    ~NativeDeviceImpl() override;

    // INativeDevicePrivate
    void INTERFACE_FUNC attachDeviceHelper(std::shared_ptr<NativeDeviceHelper> deviceHelper) override;
    void INTERFACE_FUNC updateDeviceInfo(const StringPtr& connectionString) override;
    void INTERFACE_FUNC publishConnectionStatus(ConstCharPtr statusValue) override;


    // ISerializable
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    void removed() override;

private:
    void initStatuses(const ContextPtr& ctx);

    std::shared_ptr<NativeDeviceHelper> deviceHelper;
    bool deviceInfoSet;
};

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
