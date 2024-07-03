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
#include <coreobjects/property_object.h>
#include <coretypes/listobject.h>
#include <opendaq/component.h>
#include <coreobjects/property_object.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [templated(defaultAliasName: SyncComponentPtr)]
 * [interfaceSmartPtr(ISyncComponent, GenericSyncComponentPtr)]
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
DECLARE_OPENDAQ_INTERFACE(ISyncComponent, IPropertyObject)
{
    /*!
     * @brief Retrieves the synchronization lock status.
     * @param[out] synchronizationLocked True if synchronization is locked; false otherwise.
     */
    virtual ErrCode INTERFACE_FUNC getSyncLocked(Bool* synchronizationLocked) = 0;

    /*!
     * @brief Sets the synchronization lock status.
     * @param synchronizationLocked True if synchronization is locked; false otherwise.
     */
    virtual ErrCode INTERFACE_FUNC setSyncLocked(Bool synchronizationLocked) = 0;

    /*!
     * @brief Retrieves the selected sync source interface.
     * @param[out] selectedSource The selected sync source interface.
     */
    virtual ErrCode INTERFACE_FUNC getSelectedSource(Int* selectedSource) = 0;

    /*!
     * @brief Sets the selected sync source interface.
     * @param selectedSource The selected sync source interface.
     */
    virtual ErrCode INTERFACE_FUNC setSelectedSource(Int selectedSource) = 0;

    // [elementType(interfaces, IPropertyObject)]
    /*!
     * @brief Retrieves the list of interfaces associated with this synchronization component.
     * @param[out] interface List of interfaces associated with this component.
     */
    virtual ErrCode INTERFACE_FUNC getInterfaces(IList** interfaces) = 0;

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

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, SyncComponent, ISyncComponent,
    IContext*, context
)

END_NAMESPACE_OPENDAQ