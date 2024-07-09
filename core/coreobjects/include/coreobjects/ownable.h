/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <coreobjects/property_object.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup objects_utility
 * @addtogroup objects_ownable Ownable
 * @{
 */

/*!
 * @brief An ownable object can have IPropertyObject as the owner.
 *
 * An object can declare itself ownable. When a parent object that supports a concept of ownership
 * calls the` setOwner` method, it becomes the owner of the object. It's up to the object's implementation
 * to decide what actions should it forward to the owner.
 *
 * For example, a property object that is a child of another property object will look up property values 
 * in their owner's dictionary if the property is not set locally.
 */

/*#
 * [includeHeader("<coreobjects/property_object_ptr.h>")]
 */
DECLARE_OPENDAQ_INTERFACE(IOwnable, IBaseObject)
{
    /*!
     * @brief Sets the owner of the object.
     * @param owner The object that will own this object.
     */
    virtual ErrCode INTERFACE_FUNC setOwner(IPropertyObject* owner) = 0;
};

/*!
 * @}
 */

END_NAMESPACE_OPENDAQ
