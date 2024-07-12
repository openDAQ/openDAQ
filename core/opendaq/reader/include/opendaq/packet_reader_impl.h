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
#include <opendaq/packet_reader.h>
#include <opendaq/reader_impl.h>

BEGIN_NAMESPACE_OPENDAQ

class PacketReaderImpl : public ImplementationOfWeak<IPacketReader, IInputPortNotifications>
{
public:
    explicit PacketReaderImpl(const SignalPtr& signal);
    explicit PacketReaderImpl(IInputPortConfig* port);
    ~PacketReaderImpl() override;

    ErrCode INTERFACE_FUNC getAvailableCount(SizeT* count) override;
    ErrCode INTERFACE_FUNC setOnDataAvailable(IProcedure* callback) override;

    ErrCode INTERFACE_FUNC read(IPacket** packet) override;
    ErrCode INTERFACE_FUNC readAll(IList** allPackets) override;

    // IInputPortNotifications
    ErrCode INTERFACE_FUNC acceptsSignal(IInputPort* port, ISignal* signal, Bool* accept) override;
    ErrCode INTERFACE_FUNC connected(IInputPort* port) override;
    ErrCode INTERFACE_FUNC disconnected(IInputPort* port) override;
    ErrCode INTERFACE_FUNC packetReceived(IInputPort* port) override;
    ErrCode INTERFACE_FUNC empty(Bool* empty) override;

private:
    std::mutex mutex;
    InputPortConfigPtr port;
    PropertyObjectPtr portBinder;
    ConnectionPtr connection;
    ProcedurePtr readCallback;
};

END_NAMESPACE_OPENDAQ
