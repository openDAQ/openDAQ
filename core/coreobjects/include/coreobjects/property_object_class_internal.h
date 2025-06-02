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
#include <coreobjects/property_object_class.h>
#include <coretypes/type_manager.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup objects_property_object
 * @addtogroup objects_property_object_class PropertyObjectClassInternal
 * @{
 */

/*#
 * [interfaceSmartPtr(ITypeManager, TypeManagerPtr, "<coretypes/type_manager_ptr.h>")]
 */

DECLARE_OPENDAQ_INTERFACE(IPropertyObjectClassInternal, IBaseObject)
{
    /*!
     * @brief Clones the property object class.
     * @param[out] cloned The cloned property object class.
     * @param typeManager The type manager to use for the cloned property object class. if type manager is not provided, cloned class will store a type manager from the original class.
     */
    virtual ErrCode INTERFACE_FUNC clone(IPropertyObjectClass** cloned, ITypeManager* typeManager = nullptr) = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
