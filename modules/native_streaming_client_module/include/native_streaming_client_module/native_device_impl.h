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
#include <native_streaming_client_module/device_wrapper_impl.h>

#include <config_protocol/config_protocol_client.h>

#include <native_streaming_protocol/native_streaming_client_handler.h>

#include <opendaq/context_ptr.h>
#include <opendaq/streaming_ptr.h>

#include <future>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

/// wraps the root device obtained from config protocol
/// main purpose to hold the config and transport protocol clients
/// implements custom device info

class NativeDeviceImpl final : public DeviceWrapperImpl
{
public:
    static constexpr const char* NativeConfigurationDeviceTypeId = "daq.ncd";
    static constexpr const char* NativeConfigurationDevicePrefix = "daq.ncd://";

    explicit NativeDeviceImpl(const ContextPtr& context,
                              const ComponentPtr& parent,
                              const StringPtr& connectionString,
                              const StringPtr& host,
                              const StringPtr& port,
                              const StringPtr& path);
    ~NativeDeviceImpl() override;

    // IDevice
    ErrCode INTERFACE_FUNC getInfo(IDeviceInfo** info) override;

private:
    void setupProtocolClients();
    void setDomainSignals();
    void activateStreaming();
    void validateWrapperInterfaces();

    config_protocol::PacketBuffer doConfigRequest(const config_protocol::PacketBuffer& reqPacket);
    void receiveConfigPacket(const config_protocol::PacketBuffer& packet);

    LoggerComponentPtr loggerComponent;
    StreamingPtr nativeStreaming;
    ContextPtr context;
    DeviceInfoConfigPtr deviceInfo;

    std::unique_ptr<config_protocol::ConfigProtocolClient> configProtocolClient;
    opendaq_native_streaming_protocol::NativeStreamingClientHandlerPtr transportProtocolClient;
    std::unordered_map<size_t, std::promise<config_protocol::PacketBuffer>> replyPackets;
};

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
