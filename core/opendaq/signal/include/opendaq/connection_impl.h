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
#include <opendaq/connection.h>
#include <opendaq/input_port_config_ptr.h>
#include <opendaq/context_ptr.h>
#include <coretypes/intfs.h>
#include <coretypes/weakrefobj.h>

#ifdef OPENDAQ_THREAD_SAFE
    #include <mutex>
#endif

#include <queue>

BEGIN_NAMESPACE_OPENDAQ

class ConnectionImpl : public ImplementationOfWeak<IConnection>
{
public:
    explicit ConnectionImpl(
        const InputPortPtr& port,
        const SignalPtr& signal,
        ContextPtr context
    );

    ErrCode INTERFACE_FUNC enqueue(IPacket* packet) override;
    ErrCode INTERFACE_FUNC enqueueOnThisThread(IPacket* packet) override;
    ErrCode INTERFACE_FUNC dequeue(IPacket** packet) override;
    ErrCode INTERFACE_FUNC peek(IPacket** packet) override;
    ErrCode INTERFACE_FUNC getPacketCount(SizeT* packetCount) override;
    ErrCode INTERFACE_FUNC getSignal(ISignal** signal) override;
    ErrCode INTERFACE_FUNC getInputPort(IInputPort** inputPort) override;

    ErrCode INTERFACE_FUNC getAvailableSamples(SizeT* samples) override;
    ErrCode INTERFACE_FUNC getSamplesUntilNextDescriptor(SizeT* samples) override;

    ErrCode INTERFACE_FUNC isRemote(Bool* remote) override;

    [[nodiscard]] const std::deque<PacketPtr>& getPackets() const noexcept;

#ifdef OPENDAQ_THREAD_SAFE
    template <typename Func>
    auto withLock(Func&& func) const
    {
        std::lock_guard guard(mutex);
        return func();
    }
#else
    template <typename Func>
    auto withLock(Func&& func) const
    {
        return func();
    }
#endif

private:
    InputPortConfigPtr port;
    WeakRefPtr<ISignal> signalRef;
    ContextPtr context;

#ifdef OPENDAQ_THREAD_SAFE
    mutable std::mutex mutex;
#endif

protected:
    std::deque<PacketPtr> packets;
};

END_NAMESPACE_OPENDAQ
