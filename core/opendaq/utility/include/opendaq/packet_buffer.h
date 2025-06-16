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
#include <coretypes/function.h>
#include <coretypes/listobject.h>
#include <coretypes/stringobject.h>
//#include <opendaq/data_desctiptor_ptr.h>
#include <opendaq/packet_buffer_builder.h>

BEGIN_NAMESPACE_OPENDAQ

struct IPacketBufferBuilder;

DECLARE_OPENDAQ_INTERFACE(IPacketBuffer, IBaseObject)
{
    virtual ErrCode INTERFACE_FUNC createPacket(SizeT SampleCount, IDataDescriptor* desc, IPacket* domainPacket, IPacket** packet) = 0;
    virtual ErrCode INTERFACE_FUNC getAvailableMemory(SizeT* count) = 0;
    virtual ErrCode INTERFACE_FUNC getAvailableSampleCount(SizeT* count) = 0;
    virtual ErrCode INTERFACE_FUNC resize(IPacketBufferBuilder* builder) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, PacketBuffer, IPacketBufferBuilder*, builder)

END_NAMESPACE_OPENDAQ
