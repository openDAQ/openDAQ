/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <coreobjects/property.h>

BEGIN_NAMESPACE_OPENDAQ


/*!
 * @ingroup objects_property_object
 * @addtogroup objects_property_object_obj PropertyObjectInternal
 * @{
 */

DECLARE_OPENDAQ_INTERFACE(IPropertyObjectInternal, IBaseObject)
{
    virtual ErrCode INTERFACE_FUNC checkForReferences(IProperty* property, Bool* isReferenced) = 0;
    virtual ErrCode INTERFACE_FUNC enableCoreEventTrigger() = 0;
    virtual ErrCode INTERFACE_FUNC disableCoreEventTrigger() = 0;
    virtual ErrCode INTERFACE_FUNC getCoreEventTrigger(IProcedure** trigger) = 0;
    virtual ErrCode INTERFACE_FUNC setCoreEventTrigger(IProcedure* trigger) = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
