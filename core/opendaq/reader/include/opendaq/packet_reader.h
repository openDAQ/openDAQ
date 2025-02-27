/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <opendaq/packet.h>
#include <opendaq/reader.h>
#include <coretypes/listobject.h>
#include <opendaq/signal.h>
#include <opendaq/input_port_config.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [templated(IReader)]
 * [interfaceSmartPtr(IReader, GenericReaderPtr)]
 */

/*!
 * @ingroup opendaq_readers
 * @addtogroup opendaq_packet_reader Packet reader
 * @{
 */

/*!
 * @brief A signal reader reads packets from a signal data stream.
 */
DECLARE_OPENDAQ_INTERFACE(IPacketReader, IReader)
{
    /*!
     * @brief Retrieves the next available packet in the data-stream.
     * @param[out] packet The next available packet or @c nullptr if not are available.
     */
    virtual ErrCode INTERFACE_FUNC read(IPacket** packet) = 0;

    // [elementType(packets, IPacket)]
    /*!
     * @brief Retrieves all the currently available packets in the data-stream.
     * @param[out] packets The currently available packets or an empty list.
     */
    virtual ErrCode INTERFACE_FUNC readAll(IList** packets) = 0;
};
/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, PacketReader,
    ISignal*, signal
)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, PacketReaderFromPort, IPacketReader,
    IInputPortConfig*, port
)

END_NAMESPACE_OPENDAQ
