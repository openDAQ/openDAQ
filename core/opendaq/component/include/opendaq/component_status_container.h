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

#include <coretypes/coretypes.h>
#include <opendaq/context.h>
#include <coretypes/event.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IEnumeration, EnumerationPtr, "<coretypes/enumeration_ptr.h>")]
 */

/*!
 * @ingroup opendaq_components
 * @addtogroup opendaq_component_status_container Component status container
 * @{
 */

/*!
 * @brief A container of Component Statuses and their corresponding values.
 *
 * Each status has a unique name and represents an actual status related to the openDAQ Component,
 * such as connection status, synchronization status, etc. The statuses' values are represented
 * by Enumeration objects.
 */
DECLARE_OPENDAQ_INTERFACE(IComponentStatusContainer, IBaseObject)
{
    /*!
     * @brief Gets the the current value of Component status with a given name.
     * @param name The name of Component status.
     * @param[out] value The current value of Component status.
     */
    virtual ErrCode INTERFACE_FUNC getStatus(IString* name, IEnumeration** value) = 0;

    // [templateType(statuses, IString, IEnumeration)]
    /*!
     * @brief Gets the current values of all Component statuses.
     * @param[out] statuses The Component statuses as a dictionary.
     *
     * All objects in the statuses dictionary are key value pairs of <IString, IEnumeration>.
     */
    virtual ErrCode INTERFACE_FUNC getStatuses(IDict** statuses) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, ComponentStatusContainer)

/*!@}*/

END_NAMESPACE_OPENDAQ
