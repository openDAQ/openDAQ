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
#include <coretypes/dictobject.h>
#include <coreobjects/property_info.h>
#include <coreobjects/property_object_class.h>

BEGIN_NAMESPACE_DEWESOFT_RT_CORE

/*#
 * [decorated]
 * [defaultPtr(false)]
 * [include(IPropertyObjectClass), include(IUpdatable)]
 * [templated(defaultAlias: false)]
 * [includeHeaders("<coreobjects/generic_property_info_ptr.h>", "<coreobjects/property_object_class.h>")]
 */
DECLARE_RT_INTERFACE(IPropertyObject, IBaseObject)
{
    virtual ErrCode INTERFACE_FUNC getClassName(IString** value) = 0;
    virtual ErrCode INTERFACE_FUNC setClassName(IString* value) = 0;

    virtual ErrCode INTERFACE_FUNC setProperty(IString* name, IBaseObject* value) = 0;
    virtual ErrCode INTERFACE_FUNC getProperty(IString* name, IBaseObject** value) = 0;
    virtual ErrCode INTERFACE_FUNC getPropertyEnum(IString* name, IBaseObject** value) = 0;

    virtual ErrCode INTERFACE_FUNC clearProperty(IString* name) = 0;
    virtual ErrCode INTERFACE_FUNC getPropertyInfo(IString* name, IPropertyInfo** value) = 0;
    virtual ErrCode INTERFACE_FUNC registerProperty(IPropertyInfo* value) = 0;

    // [ignore(Wrapper)]
    virtual ErrCode INTERFACE_FUNC getOnPropertyChanged(IEvent** event) = 0;

    // [elementType(list, IPropertyInfo)]
    virtual ErrCode INTERFACE_FUNC enumVisibleProperties(PropertyOrder order, IList** list) = 0;

    // [templateType(prototype, IPropertyObject)]
    virtual ErrCode INTERFACE_FUNC getPrototype(IPropertyObject** prototype) = 0;

    // [templateType(prototype, IPropertyObject)]
    virtual ErrCode INTERFACE_FUNC setPrototype(IPropertyObject* prototype) = 0;

    // [templateType(dict, IString, IBaseObject)]
    virtual ErrCode INTERFACE_FUNC getRawProperties(IDict** dict) = 0;

    // Whether the property exist in the property tree
    virtual ErrCode INTERFACE_FUNC hasProperty(IString* propName, Bool* hasProperty) = 0;

    // Whether the property exist directly on the object
    virtual ErrCode INTERFACE_FUNC isPropertySet(IString* propName, Bool* isSet) = 0;

    // [elementType(list, IPropertyInfo)]
    virtual ErrCode INTERFACE_FUNC enumProperties(PropertyOrder order, IList** list) = 0;

    virtual ErrCode INTERFACE_FUNC getPropertyConfigurationAction(IString* propName,
                                                                  ConfigurationMode mode,
                                                                  PropertyState state,
                                                                  ConfigurationAction* action) = 0;

    virtual ErrCode INTERFACE_FUNC createPropertyValue(IString* propName, IBaseObject** value) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, PropertyObject, IString*, className)

END_NAMESPACE_DEWESOFT_RT_CORE
