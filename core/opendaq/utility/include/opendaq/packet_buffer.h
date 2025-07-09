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

/*!
 * @ingroup opendaq_packet_buffers
 * @addgroup opendaq_packet_buffer PacketBuffer
 * @{
 */

/*!
 * @brief Represents an openDAQ packet buffer
 */
DECLARE_OPENDAQ_INTERFACE(IPacketBuffer, IBaseObject)
{
    /*!
     * @brief Creates a Data Packet with external memory, whose
     * @param sampleCount The amount of samples in the packet
     * @param desc Description of the samples
     * @param domainPacket A packet containing domain information
     * @param[out] packet The returned packet
     */
    virtual ErrCode INTERFACE_FUNC createPacket(SizeT sampleCount, IDataDescriptor* desc, IPacket* domainPacket, IDataPacket** packet) = 0;

    /*!
     * @brief Returns the maximum continous amount of samples either at the beginning or the end of the buffer
     * @param desc Description that contains the information about raw sample size
     * @param[out] count Returns the amount of samples that can be fitted into the space available
     */
    virtual ErrCode INTERFACE_FUNC getMaxAvailableContinousSampleCount(IDataDescriptor * desc, SizeT * count) = 0;
    // Rename to getMaxAvailableSamples

    /*!
     * @brief Returns the amount of samples (whoose raw size is gained from desc) at the end of the buffer
     * @param desc Description that contains the information about raw sample size
     * @param[out] count The available amount
     */
    virtual ErrCode INTERFACE_FUNC getAvailableSampleRight(IDataDescriptor * desc, SizeT * count) = 0;
    // Rename to getAvailableSampleCount

    /*!
     * @brief Reallocates the underlying std::vector so that it size matches the new given size
     * @param sizeInBytes The requested size of the new buffer in bytes
     */
    virtual ErrCode INTERFACE_FUNC resize(SizeT sizeInBytes) = 0;
};

/*!@}*/

/*!
 * @addtogroup opendaq_packet_buffer_factories Factories
 * @{
 */

/*!
 * @brief Creates a Packet Buffer with the given builder specifics
 * @param builder The builder that describes the specifics of the current implementation
 */
OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, PacketBuffer, IPacketBufferBuilder*, builder)

/*!@}*/

END_NAMESPACE_OPENDAQ
