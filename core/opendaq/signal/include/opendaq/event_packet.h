/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <coretypes/dictobject.h>
#include <opendaq/data_descriptor.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IPacket, GenericPacketPtr)]
 */

/*!
 * @ingroup opendaq_packets
 * @addtogroup opendaq_event_packet Event packet
 * @{
 */

/*!
 * @brief As with Data packets, Event packets travel along the signal paths. They are used to
 * notify recipients of any relevant changes to the signal sending the packet.
 */
DECLARE_OPENDAQ_INTERFACE(IEventPacket, IPacket)
{
    /*!
     * @brief Gets the ID of the event as a string. In example "DATA_DESCRIPTOR_CHANGED".
     * @param[out] id The ID of the event.
     */
    virtual ErrCode INTERFACE_FUNC getEventId(IString** id) = 0;

    // [templateType(parameters, IString, IBaseObject)]
    /*!
     * @brief Dictionary containing parameters as <String, BaseObject> pairs relevant to the event
     * signalized by the Event packet.
     * @param[out] parameters The event parameters dictionary.
     */
    virtual ErrCode INTERFACE_FUNC getParameters(IDict** parameters) = 0;
};
/*!@}*/

/*!
 * @brief Creates and Event packet with a given id and parameter dictionary.
 * @param id The ID of the event.
 * @param params The <String, BaseObject> dictionary containing the event parameters.
 */
OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, EventPacket, IString*, id, IDict*, params)

/*!
 * @brief Creates a DataDescriptorChanged Event packet.
 * @param dataDescriptor The data descriptor of the value signal.
 * @param domainDataDescriptor The data descriptor of the domain signal that carries domain data of the value signal.
 *
 * The ID of the packet is "DATA_DESCRIPTOR_CHANGED". Its parameters dictionary contains the keys "DataDescriptor"
 * and "DomainDataDescriptor", carrying their respective Signal descriptor objects as values.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY,
                                        DataDescriptorChangedEventPacket,
                                        IEventPacket,
                                        IDataDescriptor*,
                                        dataDescriptor,
                                        IDataDescriptor*,
                                        domainDataDescriptor)

END_NAMESPACE_OPENDAQ
