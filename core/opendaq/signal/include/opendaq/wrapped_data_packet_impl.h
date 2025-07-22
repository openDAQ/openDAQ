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
#include <opendaq/data_packet_ptr.h>
#include <opendaq/data_packet_impl.h>
#include <opendaq/wrapped_data_packet.h>

BEGIN_NAMESPACE_OPENDAQ

class WrappedDataPacketImpl : public DataPacketImpl<IDataPacket, IWrappedDataPacket>
{
public:
    using Super = DataPacketImpl<IDataPacket, IWrappedDataPacket>;

    explicit WrappedDataPacketImpl(IDataPacket* sourcePacket,
                                   IDataDescriptor* descriptor);

    ErrCode INTERFACE_FUNC getDomainPacket(IDataPacket** packet) override;
    ErrCode INTERFACE_FUNC getOffset(INumber** offset) override;

    ErrCode INTERFACE_FUNC getSourcePacket(IDataPacket** sourcePacket) override;

protected:
    void internalDispose([[maybe_unused]] bool disposing) override;

private:
    DataPacketPtr sourcePacket;
};

END_NAMESPACE_OPENDAQ
