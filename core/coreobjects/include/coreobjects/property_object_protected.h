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
#include <coretypes/baseobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup objects_property_object
 * @addtogroup objects_property_object_protected PropertyObjectProtected
 * @{
 */

/*!
 * @brief Provides protected access that allows changing read-only property values of a Property object.
 */
DECLARE_OPENDAQ_INTERFACE(IPropertyObjectProtected, IBaseObject)
{
    /*!
     * @brief Sets a property value. Does not fail if the property is read-only.
     * @param propertyName The name of the Property of which value the function should set.
     * @param value The property value to set.
     */
    virtual ErrCode INTERFACE_FUNC setProtectedPropertyValue(IString* propertyName, IBaseObject* value) = 0;

    /*!
     * @brief Clears a property value. Does not fail if the property is read-only.
     * @param propertyName The name of the Property of which value the function should be cleared.
     */
    virtual ErrCode INTERFACE_FUNC clearProtectedPropertyValue(IString* propertyName) = 0;
};

/*!
 * @}
 */

END_NAMESPACE_OPENDAQ
