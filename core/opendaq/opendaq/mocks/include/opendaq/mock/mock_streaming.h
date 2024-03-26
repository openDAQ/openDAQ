/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <opendaq/streaming_impl.h>
#include <opendaq/streaming.h>

class MockStreamingImpl : public daq::Streaming
{
public:
    explicit MockStreamingImpl(const daq::StringPtr& connectionString);

protected:
    void onSetActive(bool active) override;
    daq::StringPtr onGetSignalStreamingId(const daq::StringPtr& signalRemoteId) override;
    void onAddSignal(const daq::MirroredSignalConfigPtr& signal) override;
    void onRemoveSignal(const daq::MirroredSignalConfigPtr& signal) override;
    void onSubscribeSignal(const daq::StringPtr& signalRemoteId, const daq::StringPtr& domainSignalRemoteId) override;
    void onUnsubscribeSignal(const daq::StringPtr& signalRemoteId, const daq::StringPtr& domainSignalRemoteId) override;
    daq::EventPacketPtr onCreateDataDescriptorChangedEventPacket(const daq::StringPtr& signalRemoteId) override;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY,
    MockStreaming, daq::IStreaming,
    daq::IString*, connectionString
)
