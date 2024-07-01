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
#include <opendaq/device_impl.h>
#include "websocket_streaming/streaming_client.h"
#include <opendaq/signal_ptr.h>
#include <opendaq/streaming_ptr.h>

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING

class WebsocketClientDeviceImpl : public Device
{
public:
    explicit WebsocketClientDeviceImpl(const ContextPtr& ctx,
                                       const ComponentPtr& parent,
                                       const StringPtr& localId,
                                       const StringPtr& connectionString);

protected:
    void removed() override;

    DeviceInfoPtr onGetInfo() override;
    void createWebsocketStreaming();
    void activateStreaming();
    void updateSignalProperties(const SignalPtr& signal, const SubscribedSignalInfo& sInfo);
    void onSignalInit(const StringPtr& signalId, const SubscribedSignalInfo& sInfo);
    void onSignalUpdated(const StringPtr& signalId, const SubscribedSignalInfo& sInfo);
    void onDomainSignalInit(const StringPtr& signalId, const StringPtr& domainSignalId);
    void registerAvailableSignals(const std::vector<std::string>& signalIds);
    void removeSignals(const std::vector<std::string>& signalIds);
    void registerHiddenSignal(const StringPtr& signalId, const SubscribedSignalInfo& sInfo);
    void addInitializedSignals();

    DeviceInfoConfigPtr deviceInfo;
    std::unordered_map<StringPtr, SignalPtr> deviceSignals;
    StreamingPtr websocketStreaming;
    StringPtr connectionString;
    std::vector<std::string> orderedSignalIds;
};

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING
