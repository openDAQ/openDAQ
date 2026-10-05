/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
    explicit MockStreamingImpl(const daq::StringPtr& connectionString, const daq::ContextPtr& context);

    MockStreamingImpl(const daq::StringPtr& connectionString,
                      const daq::ContextPtr& context,
                      const daq::StringPtr& protocolId,
                      const daq::StringPtr& protocolGroupId);

protected:
    void onSetActive(bool active) override;
    void onAddSignal(const daq::MirroredSignalConfigPtr& signal) override;
    void onRemoveSignal(const daq::MirroredSignalConfigPtr& signal) override;
    void onSubscribeSignal(const daq::StringPtr& signalStreamingIdd) override;
    void onUnsubscribeSignal(const daq::StringPtr& signalStreamingId) override;
    void onRegisterStreamedClientSignal(const daq::SignalPtr& signal) override;
    void onUnregisterStreamedClientSignal(const daq::SignalPtr& signal) override;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY,
    MockStreaming, daq::IStreaming,
    daq::IString*, connectionString,
    daq::IContext*, context
)

// Same mock, but with a caller-defined protocol id and protocol group id
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    INTERNAL_FACTORY,
    MockStreamingWithProtocol, daq::IStreaming,
    createMockStreamingWithProtocol,
    daq::IString*, connectionString,
    daq::IContext*, context,
    daq::IString*, protocolId,
    daq::IString*, protocolGroupId
)
