/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <opendaq/data_descriptor.h>
#include <opendaq/deleter.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Creates a binary value packet that contains exactly one sample.
 * @param descriptor The descriptor of the signal sending the data.
 * @param sampleMemSize The memory size of a sample.
 *
 * Binary value packet should contain exactly one sample of `SampleType::Binary`
 * sample type.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, BinaryDataPacket,
    IDataPacket, createBinaryDataPacket,
    IDataPacket*, domainPacket,
    IDataDescriptor*, descriptor,
    SizeT, sampleMemSize
)

/*!
 * @brief Creates a binary value packet with external memory and a deleter that contains exactly one sample.
 * @param descriptor The descriptor of the signal sending the data.
 * @param sampleMemSize The memory size of a sample.
 *
 * Binary value packet should contain exactly one sample of `SampleType::Binary`
 * sample type. Memory should be allocated by the caller and a custom deleter callback
 * should be provided.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, BinaryDataPacketWithExternalMemory,
    IDataPacket, createBinaryDataPacketWithExternalMemory,
    IDataPacket*, domainPacket,
    IDataDescriptor*, descriptor,
    SizeT, sampleMemSize,
    void*, data,
    IDeleter*, deleter
)

END_NAMESPACE_OPENDAQ
