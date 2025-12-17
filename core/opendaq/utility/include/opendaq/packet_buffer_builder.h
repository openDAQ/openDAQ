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
#include <opendaq/packet_buffer.h>
#include <opendaq/context.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_packet_buffer_builders
 * @addtogroup packet_buffer_builder PacketBufferBuilder
 * @{
 */

/*!
 * @brief Briefly
 */
DECLARE_OPENDAQ_INTERFACE(IPacketBufferBuilder, IBaseObject)
{
    /*!
     * @brief Gets the context pointer
     * @param[out] context The returned ContextPtr
     */
    virtual ErrCode INTERFACE_FUNC getContext(IContext** context) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the context pointer
     * @param context The pointer that will be set
     */
    virtual ErrCode INTERFACE_FUNC setContext(IContext* context) = 0;

    /*!
     * @brief Gets the size of the underlying buffer in bytes
     * @param[out] sizeInBytes Value of this variable will contain the size of the buffer in bytes
     */
    virtual ErrCode INTERFACE_FUNC getSizeInBytes(SizeT* sizeInBytes) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the size of the underlying buffer in bytes
     * @param sizeInBytes Sets the size of the buffer in bytes
     */
    virtual ErrCode INTERFACE_FUNC setSizeInBytes(SizeT sizeInBytes) = 0;

    /*!
     * @brief Builds the Packet Buffer with the internally specified size and context
     * @param[out] buffer Returns the newly created buffer
     */
    virtual ErrCode INTERFACE_FUNC build(IPacketBuffer** buffer) = 0;
};

/*!@}*/

/*!
 * @brief Create an empty default PacketBufferBuilder (default sizeInBytes in set to 0)
 */
OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, PacketBufferBuilder)

END_NAMESPACE_OPENDAQ
