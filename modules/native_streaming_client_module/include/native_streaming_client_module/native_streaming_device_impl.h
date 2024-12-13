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
#include <opendaq/device_impl.h>
#include <opendaq/streaming_ptr.h>
#include <native_streaming_protocol/native_streaming_client_handler.h>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE

static const char* NativeStreamingDeviceTypeId = "OpenDAQNativeStreaming";
static const char* NativeStreamingDevicePrefix = "daq.ns";

class NativeStreamingDeviceImpl : public Device
{
public:
    explicit NativeStreamingDeviceImpl(const ContextPtr& ctx,
                                       const ComponentPtr& parent,
                                       const StringPtr& localId,
                                       const StringPtr& connectionString,
                                       opendaq_native_streaming_protocol::NativeStreamingClientHandlerPtr transportProtocolClient,
                                       std::shared_ptr<boost::asio::io_context> processingIOContextPtr,
                                       Int initTimeout);

protected:
    void removed() override;
    DeviceInfoPtr onGetInfo() override;

    void signalAvailableHandler(const StringPtr& signalStringId, const StringPtr& serializedSignal);
    void signalUnavailableHandler(const StringPtr& signalStringId);
    void connectionStatusChangedHandler(const EnumerationPtr& status);
    void publishConnectionStatus();
    void createNativeStreaming(opendaq_native_streaming_protocol::NativeStreamingClientHandlerPtr transportProtocolClient,
                               std::shared_ptr<boost::asio::io_context> processingIOContextPtr,
                               Int initTimeout);
    void activateStreaming();
    void addToDeviceSignals(const StringPtr& signalStringId, const StringPtr& serializedSignal);
    void addToDeviceSignalsOnReconnection(const StringPtr& signalStringId, const StringPtr& serializedSignal);
    SignalPtr createSignal(const StringPtr& signalStringId, const StringPtr& serializedSignal);

    StringPtr connectionString;
    EnumerationPtr connectionStatus;
    StreamingPtr nativeStreaming;
    std::unordered_map<StringPtr, std::pair<SignalPtr, StringPtr>, StringHash, StringEqualTo> deviceSignals;
    std::unordered_map<StringPtr, std::pair<SignalPtr, StringPtr>, StringHash, StringEqualTo> deviceSignalsReconnection;
};

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_CLIENT_MODULE
