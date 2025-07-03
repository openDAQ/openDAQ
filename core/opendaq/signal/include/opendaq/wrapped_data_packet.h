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

/*#
 * [interfaceSmartPtr(IDataPacket, GenericDataPacketPtr)]
 */

/*!
 * @ingroup opendaq_packets
 * @addtogroup opendaq_data_packet Wrapped data packet
 * @{
 */

/*!
 * @brief Data packet that wraps the source packet.
 *
 * Wrapped data packets share the same raw data with source packet but have their own data descriptor.
 * This typically means that their raw sample type and sample count should be the same.
 */
DECLARE_OPENDAQ_INTERFACE(IWrappedDataPacket, IBaseObject)
{
    // [templateType(packet, IDataPacket)]
    /*!
     * @brief Gets the wrapped source data packet.
     * @param[out] sourcePacket The wrapped source data packet.
     */
    virtual ErrCode INTERFACE_FUNC getSourcePacket(IDataPacket** sourcePacket) = 0;
};

/*!@}*/

/*!
 * @brief Creates a wrapped data packet.
 * @param sourcePacket The source packet being wrapped.
 * @param descriptor The descriptor of the packet.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(LIBRARY_FACTORY,
                                                            WrappedDataPacket,
                                                            IDataPacket,
                                                            createWrappedDataPacket,
                                                            IDataPacket*,
                                                            sourcePacket,
                                                            IDataDescriptor*,
                                                            descriptor)

END_NAMESPACE_OPENDAQ
