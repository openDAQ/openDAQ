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
#include <opendaq/data_packet.h>

BEGIN_NAMESPACE_OPENDAQ

extern "C" PUBLIC_EXPORT ErrCode daqBulkCreateDataPackets(
    IDataPacket** dataPackets,
    IDataDescriptor** valueDescriptors,
    IDataDescriptor** domainDescriptors,
    size_t* sampleCount,
    int64_t* offsets,
    size_t count,
    size_t dataAlign);

extern "C" PUBLIC_EXPORT ErrCode daqCreateValuePacketWithImplicitDomainPacket(
    IDataPacket** dataPacket,
    IDataDescriptor* valueDescriptor,
    IDataDescriptor* domainDescriptor,
    size_t sampleCount,
    int64_t offset,
    size_t dataAlign);

extern "C" PUBLIC_EXPORT ErrCode daqCreateValuePacketWithExplicitDomainPacket(
    IDataPacket** dataPacket,
    IDataDescriptor* valueDescriptor,
    IDataDescriptor* domainDescriptor,
    size_t sampleCount,
    size_t dataAlign);

END_NAMESPACE_OPENDAQ
