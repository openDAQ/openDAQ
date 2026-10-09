/*
 * Copyright 2022-2026 openDAQ d.o.o.
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

#include <coretypes/baseobject.h>
#include <opendaq/sync_interface.h>
#include <coretypes/stringobject.h>
#include <coretypes/dictobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_synchronization_path
 * @addtogroup opendaq_synchronization Synchronization
 * @{
 */

/*!
 * @brief Interface representing the Synchronization of a device in a Test & Measurement system.
 *
 * The device holds it as the hidden property "daqSynchronization". IDevice::getSynchronization
 * returns null when the device is not clocked.
 */
DECLARE_OPENDAQ_INTERFACE(ISynchronization, IBaseObject)
{
    /*!
     * @brief Gets all synchronization interfaces registered with this synchronization.
     * @param[out] interfaces A dictionary mapping interface IDs to the sync interfaces themselves.
     */
    // [templateType(interfaces, IString, ISyncInterface)]
    virtual ErrCode INTERFACE_FUNC getInterfaces(IDict** interfaces) = 0;

    /*!
     * @brief Gets the registered synchronization interfaces that can be selected as the
     * synchronization source, which are those whose `getSourceSupported` is true.
     * @param[out] sources A dictionary mapping interface IDs to the sync interfaces that can be
     * selected as the synchronization source.
     */
    // [templateType(sources, IString, ISyncInterface)]
    virtual ErrCode INTERFACE_FUNC getAvailableSources(IDict** sources) = 0;

    /*!
     * @brief Selects the synchronization interface with the given ID as the synchronization source.
     * @param sourceName The ID of the synchronization interface to select as the source.
     *
     * The new source is set to `Auto`, or to `Input` if `Auto` is unavailable, and the previous
     * source is set to `Off`. Fails if `sourceName` is not the ID of an available source. On
     * failure the previous source is restored.
     */
    virtual ErrCode INTERFACE_FUNC setSource(IString* sourceName) = 0;

    /*!
     * @brief Gets the currently selected synchronization source.
     * @param[out] source The currently selected synchronization interface.
     */
    virtual ErrCode INTERFACE_FUNC getSource(ISyncInterface** source) = 0;

    /*!
     * @brief Gets the reference domain IDs of all interfaces not in `Off`: the source and the
     * active outputs.
     * @param[out] ids The list of reference domain IDs.
     *
     * Interfaces without a reference domain ID are left out. Describes the synchronization
     * service; for the state applied to data use IReferenceDomainInfo::getReferenceDomainIds.
     */
    // [templateType(ids, IString)]
    virtual ErrCode INTERFACE_FUNC getReferenceDomainIds(IList** ids) = 0;
};
/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, Synchronization, ISynchronization,
    ITypeManager*, manager,
    IString*, deviceId)

END_NAMESPACE_OPENDAQ
