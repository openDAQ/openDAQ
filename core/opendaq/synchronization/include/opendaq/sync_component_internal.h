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

#include <coretypes/listobject.h>
#include <coreobjects/property_object.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, GenericPropertyObjectPtr, "<coreobjects/property_object_ptr.h>")]
 */

/*!
 * @ingroup opendaq_synchronization_path
 * @addtogroup opendaq_sync_component Sync Component
 * @{
 */

/*!
 * @brief Interface representing a Synchronization Component in a Test & Measurement system.
 * A SynchronizationComponent ensures synchronization among measurement devices in the system.
 * It can act as a sync source and/or as a sync output, with each component having one sync input
 * and 0 to n sync outputs.
 *
 * SynchronizationComponents are configured via interfaces, which can include PTP, IRIQ, GPS,
 * and CLK sync interfaces, among others.
 *
 * @note Every SynchronizationComponent has at least one interface. Only one interface can be set
 * as an input, while others can be used as sync outputs to synchronize other devices.
 * The configuration of these interfaces and the reading of their status is defined in Part 4.
 *
 * @note Depending on the setup, some interfaces may be switched off, and some interfaces may
 * act as sync sources or outputs.
 *
 * @note A CLK interface can be used to let a device run in Fre-Run mode, where the device
 * syncs internally to an internal quartz.
 */
DECLARE_OPENDAQ_INTERFACE(ISyncComponentInternal, IBaseObject)
{
    /*!
     * @brief Sets the synchronization lock status.
     * @param synchronizationLocked True if synchronization is locked; false otherwise.
     */
    virtual ErrCode INTERFACE_FUNC setSyncLocked(Bool synchronizationLocked) = 0;

    // [elementType(interface, IPropertyObject)]
    /*!
     * @brief Adds an interface to the synchronization component.
     * @param interface The interface to be added.
     */
    virtual ErrCode INTERFACE_FUNC addInterface(IPropertyObject* interface) = 0;

    /*!
     * @brief Removes an interface from the synchronization component.
     * @param interfaceName The name of the interface to be removed.
     */
    virtual ErrCode INTERFACE_FUNC removeInterface(IString* interfaceName) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ