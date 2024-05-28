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
#include <coreobjects/component_type.h>
#include <coretypes/stringobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup objects_utility
 * @addtogroup objects_component_type Component type builder
 * @{
 */

enum class ComponentTypeSort
{
    Undefined = 0,
    Server = 1,
    Device,
    FunctionBlock,
    Streaming
};

/*#
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceLibrary(IComponentType, "coreobjects")]
 */

/*!
 * @brief Builder component of Component type objects. Contains setter methods to configure the Component type parameters, and a
 * `build` method that builds the object.
 *
 * Depending on the set "type" builder parameter, a different Component type is created - eg. Streaming type,
 * Device type, Function block type, or Server type
 */
DECLARE_OPENDAQ_INTERFACE(IComponentTypeBuilder, IBaseObject)
{
    /*!
     * @brief Builds and returns a Component type object using the currently set values of the Builder.
     * @param[out] componentType The built Component type.
     *
     * Depending on the set "sort" builder parameter, a different Component type is created - eg. Streaming type,
     * Device type, Function block type, or Server type
     */
    virtual ErrCode INTERFACE_FUNC build(IComponentType** componentType) = 0;
    
    // [returnSelf]
    virtual ErrCode INTERFACE_FUNC setId(IString* id) = 0;
    virtual ErrCode INTERFACE_FUNC getId(IString** id) = 0;

    // [returnSelf]
    virtual ErrCode INTERFACE_FUNC setTypeSort(ComponentTypeSort sort) = 0;
    virtual ErrCode INTERFACE_FUNC getTypeSort(ComponentTypeSort* sort) = 0;

    // [returnSelf]
    virtual ErrCode INTERFACE_FUNC setName(IString* name) = 0;
    virtual ErrCode INTERFACE_FUNC getName(IString** name) = 0;

    // [returnSelf]
    virtual ErrCode INTERFACE_FUNC setDescription(IString* description) = 0;
    virtual ErrCode INTERFACE_FUNC getDescription(IString** description) = 0;

    // [returnSelf]
    virtual ErrCode INTERFACE_FUNC setConnectionStringPrefix(IString* prefix) = 0;
    virtual ErrCode INTERFACE_FUNC getConnectionStringPrefix(IString** prefix) = 0;

    // [returnSelf]
    virtual ErrCode INTERFACE_FUNC setDefaultConfig(IPropertyObject* defaultConfig) = 0;
    virtual ErrCode INTERFACE_FUNC getDefaultConfig(IPropertyObject** defaultConfig) = 0;
};
/*!@}*/

/*!
 * @ingroup objects_component_type
 * @addtogroup objects_component_type_factories Factories
 * @{
 */

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, ComponentTypeBuilder, IComponentTypeBuilder)
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, DeviceTypeBuilder, IComponentTypeBuilder)
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, StreamingTypeBuilder, IComponentTypeBuilder)
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, ServerTypeBuilder, IComponentTypeBuilder)
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, FunctionBlockTypeBuilder, IComponentTypeBuilder)

/*!@}*/

END_NAMESPACE_OPENDAQ
