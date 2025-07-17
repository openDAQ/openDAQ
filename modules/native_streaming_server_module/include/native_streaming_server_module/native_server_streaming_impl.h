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
#include <opendaq/streaming_impl.h>
#include <native_streaming_server_module/common.h>

#include <native_streaming_protocol/native_streaming_server_handler.h>
#include <boost/asio/dispatch.hpp>

BEGIN_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE

using TransportLevelHandlerPtr = std::shared_ptr<opendaq_native_streaming_protocol::NativeStreamingServerHandler>;

class NativeServerStreamingImpl : public daq::Streaming
{
public:
    explicit NativeServerStreamingImpl(TransportLevelHandlerPtr transportServerHandler, ContextPtr context);

    ~NativeServerStreamingImpl();

protected:
    void onSetActive(bool active) override;
    void onAddSignal(const MirroredSignalConfigPtr& signal) override;
    void onRemoveSignal(const MirroredSignalConfigPtr& signal) override;
    void onSubscribeSignal(const StringPtr& signalStreamingId) override;
    void onUnsubscribeSignal(const StringPtr& signalStreamingId) override;

    TransportLevelHandlerPtr transportServerHandler;
};

END_NAMESPACE_OPENDAQ_NATIVE_STREAMING_SERVER_MODULE
