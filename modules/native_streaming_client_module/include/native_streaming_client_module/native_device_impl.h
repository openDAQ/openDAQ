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

#include <native_streaming_client_module/common.h>

#include <config_protocol/config_protocol_client.h>
#include <config_protocol/config_client_device_impl.h>

#include <native_streaming_protocol/native_streaming_client_handler.h>

#include <opendaq/context_ptr.h>
#include <opendaq/streaming_ptr.h>

#include <future>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

static const char* NativeConfigurationDeviceTypeId = "daq.nd";
static const char* NativeConfigurationDevicePrefix = "daq.nd://";

class NativeDeviceImpl;

class NativeDeviceHelper
{
public:
    explicit NativeDeviceHelper(const ContextPtr& context,
                                opendaq_native_streaming_protocol::NativeStreamingClientHandlerPtr transportProtocolClient);
    ~NativeDeviceHelper();

    DevicePtr connectAndGetDevice(const ComponentPtr& parent);

    void subscribeToCoreEvent(const ContextPtr& context);
    void unsubscribeFromCoreEvent(const ContextPtr& context);

    void addStreaming(const StreamingPtr& streaming);

private:
    void setupProtocolClients(const ContextPtr& context);
    config_protocol::PacketBuffer doConfigRequest(const config_protocol::PacketBuffer& reqPacket);
    void receiveConfigPacket(const config_protocol::PacketBuffer& packet);
    void coreEventCallback(ComponentPtr& sender, CoreEventArgsPtr& eventArgs);
    void componentAdded(const ComponentPtr& sender, const CoreEventArgsPtr& eventArgs);
    void addSignalsToStreaming(const ListPtr<ISignal>& signals);

    LoggerComponentPtr loggerComponent;
    std::unique_ptr<config_protocol::ConfigProtocolClient<NativeDeviceImpl>> configProtocolClient;
    opendaq_native_streaming_protocol::NativeStreamingClientHandlerPtr transportProtocolClient;
    std::unordered_map<size_t, std::promise<config_protocol::PacketBuffer>> replyPackets;
    StreamingPtr streaming;
    WeakRefPtr<IDevice> deviceRef;
};

DECLARE_OPENDAQ_INTERFACE(INativeDevicePrivate, IBaseObject)
{
    virtual void INTERFACE_FUNC attachDeviceHelper(std::unique_ptr<NativeDeviceHelper> deviceHelper) = 0;
    virtual void INTERFACE_FUNC setConnectionString(const StringPtr& connectionString) = 0;
};

class NativeDeviceImpl final : public config_protocol::GenericConfigClientDeviceImpl<config_protocol::ConfigClientDeviceBase<INativeDevicePrivate>>
{
public:
    using Super = config_protocol::GenericConfigClientDeviceImpl<config_protocol::ConfigClientDeviceBase<INativeDevicePrivate>>;

    explicit NativeDeviceImpl(const config_protocol::ConfigProtocolClientCommPtr& configProtocolClientComm,
                              const std::string& remoteGlobalId,
                              const ContextPtr& ctx,
                              const ComponentPtr& parent,
                              const StringPtr& localId);
    ~NativeDeviceImpl() override;

    // IDevice
    ErrCode INTERFACE_FUNC getInfo(IDeviceInfo** info) override;

    // INativeDevicePrivate
    void INTERFACE_FUNC attachDeviceHelper(std::unique_ptr<NativeDeviceHelper> deviceHelper) override;
    void INTERFACE_FUNC setConnectionString(const StringPtr& connectionString) override;

    // ISerializable
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

private:
    DeviceInfoConfigPtr deviceInfo;
    std::unique_ptr<NativeDeviceHelper> deviceHelper;
};

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
