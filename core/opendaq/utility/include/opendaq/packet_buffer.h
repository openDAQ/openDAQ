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
#include <opendaq/data_descriptor.h>
#include <opendaq/data_packet.h>

BEGIN_NAMESPACE_OPENDAQ

struct IPacketBufferBuilder;

DECLARE_OPENDAQ_INTERFACE(IPacketBuffer, IBaseObject)
{
    // SampleCount should be an in-out parameter
    virtual ErrCode INTERFACE_FUNC createPacket(SizeT sampleCount, IDataDescriptor* desc, IPacket* domainPacket, IDataPacket** packet) = 0;

    // What does this return? Consecutive available memory, or total available memory including wraparound?
    // Not really needed, not useful in end-user examples
    virtual ErrCode INTERFACE_FUNC getAvailableMemory(SizeT* count) = 0;

    virtual ErrCode INTERFACE_FUNC getMaxAvailableContinousSampleCount(IDataDescriptor * desc, SizeT * count) = 0;
    virtual ErrCode INTERFACE_FUNC getAvailableContinousSampleLeft(IDataDescriptor * desc, SizeT * count) = 0;
    virtual ErrCode INTERFACE_FUNC getAvailableContinousSampleRight(IDataDescriptor * desc, SizeT * count) = 0;


    // getMaxLinearAvailableSampleCount // getMaxAvailableContionousSampleCount
    virtual ErrCode INTERFACE_FUNC getAvailableSampleCount(IDataDescriptor* desc, SizeT* count) = 0;
    virtual ErrCode INTERFACE_FUNC resize(SizeT sizeInBytes) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, PacketBuffer, IPacketBufferBuilder*, builder)

END_NAMESPACE_OPENDAQ
