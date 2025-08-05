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

#include <functional>

#include <opendaq/device_impl.h>
#include <opendaq/opendaq.h>

#include <websocket_streaming/common.h>
#include <websocket_streaming/ws_streaming.h>
#include <websocket_streaming/ws_streaming_device.h>
#include <websocket_streaming/ws_streaming_signal.h>

using namespace std::placeholders;

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

DeviceTypePtr WsStreamingDevice::OLD_TYPE = DeviceTypeBuilder()
    .setId("OpenDAQLTStreamingOld")
    .setName("Streaming LT enabled pseudo-device")
    .setDescription("Exposes signals from devices streamed using the WebSocket Streaming Protocol")
    .setConnectionStringPrefix("daq.ws")
    .build();

DeviceTypePtr WsStreamingDevice::NEW_TYPE = DeviceTypeBuilder()
    .setId("OpenDAQLTStreaming")
    .setName("Streaming LT enabled pseudo-device")
    .setDescription("Exposes signals from devices streamed using the WebSocket Streaming Protocol")
    .setConnectionStringPrefix("daq.lt")
    .build();

WsStreamingDevice::WsStreamingDevice(
        const ContextPtr& context,
        const ComponentPtr& parent,
        const StringPtr& localId,
        const StringPtr& connectionString)
    : Device(context, parent, localId)
    , connectionString(connectionString)
{
    if (!connectionString.assigned())
        DAQ_THROW_EXCEPTION(ArgumentNullException, "connectionString cannot be null");
    name = "WebsocketClientPseudoDevice";

    streaming = createWithImplementation<IStreaming, WsStreaming>(connectionString, context);
    streaming.setActive(true);

    auto& wsStreaming = *reinterpret_cast<WsStreaming *>(streaming.getObject());

    streamingEvents.emplace_back(wsStreaming.onSignalAvailable.connect(std::bind(&WsStreamingDevice::onSignalAvailable, this, _1, _2, _3)));
    streamingEvents.emplace_back(wsStreaming.onSignalUnavailable.connect(std::bind(&WsStreamingDevice::onSignalUnavailable, this, _1)));
}

void WsStreamingDevice::removed()
{
    streamingEvents.clear();

    streaming.release();

    Device::removed();
}

DeviceInfoPtr WsStreamingDevice::onGetInfo()
{
    return DeviceInfo(connectionString, "WebsocketClientPseudoDevice");
}

void WsStreamingDevice::onSignalAvailable(
    wss::remote_signal_ptr signal,
    const DataDescriptorPtr& valueDescriptor,
    const DataDescriptorPtr& domainDescriptor)
{
    auto openDaqDomainSignal = createWithImplementation<IMirroredSignalPrivate, WsStreamingSignal>(
        context,
        signals,
        signal->id() + ".Time");
    openDaqDomainSignal.setMirroredDataDescriptor(domainDescriptor);

    auto openDaqSignal = createWithImplementation<IMirroredSignalPrivate, WsStreamingSignal>(
        context,
        signals,
        signal->id());
    openDaqSignal.setMirroredDataDescriptor(valueDescriptor);
    openDaqSignal.setMirroredDomainSignal(openDaqDomainSignal);

    streaming.addSignals({openDaqSignal});

    auto mirroredSignalConfigPtr = openDaqSignal.asPtr<IMirroredSignalConfig>();
    mirroredSignalConfigPtr.setActiveStreamingSource(streaming.getConnectionString());

    addSignal(openDaqSignal);
    streamingSignals[signal->id()] = openDaqSignal;
}

void WsStreamingDevice::onSignalUnavailable(wss::remote_signal_ptr signal)
{
    auto it = streamingSignals.find(signal->id());
    if (it == streamingSignals.end())
        return;

    removeSignal(it->second);
    streamingSignals.erase(it);
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
