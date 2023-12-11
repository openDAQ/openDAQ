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
#include <opendaq/packet_reader.h>
#include <opendaq/reader_impl.h>

BEGIN_NAMESPACE_OPENDAQ

class PacketReaderImpl : public ImplementationOf<IPacketReader>
{
public:
    explicit PacketReaderImpl(const SignalPtr& signal);
    explicit PacketReaderImpl(IInputPortConfig* port);
    ~PacketReaderImpl() override;

    ErrCode INTERFACE_FUNC getAvailableCount(SizeT* count) override;
    ErrCode INTERFACE_FUNC setOnDescriptorChanged(IFunction* callback) override;

    ErrCode INTERFACE_FUNC read(IPacket** packet) override;
    ErrCode INTERFACE_FUNC readAll(IList** allPackets) override;
private:
    std::mutex mutex;
    InputPortConfigPtr port;
    ConnectionPtr connection;
};

END_NAMESPACE_OPENDAQ
