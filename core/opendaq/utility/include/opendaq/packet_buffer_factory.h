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
#include <opendaq/packet_buffer_ptr.h>
#include <opendaq/packet_buffer_builder_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
* @ingroup opendaq_packet_buffers
* @addtogroup opendaq_packet_buffer_factories Factories
*/

/*!
 * @brief Wrapper pointer for PacketBuffer
 */
inline PacketBufferPtr PacketBuffer(const PacketBufferBuilderPtr& builder)
{
    return PacketBufferPtr(PacketBuffer_Create(builder));
}

END_NAMESPACE_OPENDAQ
