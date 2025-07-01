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
     * @param sampleCount Lskjbv
     * @param desc Ajlkajbv
     * @param domainPacket Djkjlkj
     * @param[out] packet The returned packet
     */
    virtual ErrCode INTERFACE_FUNC createPacket(SizeT sampleCount, IDataDescriptor* desc, IPacket* domainPacket, IDataPacket** packet) = 0;

    virtual ErrCode INTERFACE_FUNC getAvailableMemory(SizeT* count) = 0;
    virtual ErrCode INTERFACE_FUNC getAvailableSampleCount(IDataDescriptor* desc, SizeT* count) = 0;

    /*!
     * @brief Blah blah
     * @param desc Stuff written here
     * @param[out] count Check...
     */
    virtual ErrCode INTERFACE_FUNC getMaxAvailableContinousSampleCount(IDataDescriptor * desc, SizeT * count) = 0;

    /*!
     * @brief Another block of stuff
     * @param desc Something that goes in
     * @param[out] count Something that goes out
     */
    virtual ErrCode INTERFACE_FUNC getAvailableContinousSampleLeft(IDataDescriptor * desc, SizeT * count) = 0;

    /*!
     * @brief Here more written stuff
     * @param desc Out here
     * @param[out] count In there
     */
    virtual ErrCode INTERFACE_FUNC getAvailableContinousSampleRight(IDataDescriptor * desc, SizeT * count) = 0;

    /*!
     * @brief Write something interesting
     * @param sizeInBytes Sufff
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
