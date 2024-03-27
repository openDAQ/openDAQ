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

#include <coretypes/coretypes.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IEnumeration, EnumerationPtr, "<coretypes/enumeration_ptr.h>")]
 */

/*!
 * @ingroup opendaq_components
 * @addtogroup opendaq_component_status_container Component status container private
 * @{
 */

/*!
 * @brief Provides access to private methods of the Component status container.
 *
 * Said methods allow for adding new statuses and setting a value for existing statuses stored in
 * the component status container.
 *
 * Status changed Core events are triggered whenever there is a change in the status of the openDAQ Component.
 */
DECLARE_OPENDAQ_INTERFACE(IComponentStatusContainerPrivate, IBaseObject)
{
    /*!
     * @brief Adds the new status with given name and initial value.
     * @param name The name of the component status.
     * @param initialValue The initial value of the component status.
     */
    virtual ErrCode INTERFACE_FUNC addStatus(IString* name, IEnumeration* initialValue) = 0;

    /*!
     * @brief Sets the value for the existing component status.
     * @param name The name of the component status.
     * @param value The new value of the component status.
     */
    virtual ErrCode INTERFACE_FUNC setStatus(IString* name, IEnumeration* value) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
