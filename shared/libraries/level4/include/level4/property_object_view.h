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
#include <coretypes/stringobject.h>
#include <coreobjects/property_object.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(IComponent, "opendaq")]
 * [interfaceLibrary(ICoreEventArgs, "coreobjects")]
 */

/*!
 * @brief Represents a view of a PropertyObject.
 *
 * A view is a separate PropertyObject that contains standardized properties
 * (e.g., Level 4 specification) and maintains a reference to the original owner object.
 */
DECLARE_OPENDAQ_INTERFACE(IPropertyObjectView, IBaseObject)
{
    /*!
     * @brief Gets the original PropertyObject being viewed.
     * @param[out] propObject The original property object.
     */
    virtual ErrCode INTERFACE_FUNC getViewOwner(IPropertyObject** propObject) = 0;
};

END_NAMESPACE_OPENDAQ
