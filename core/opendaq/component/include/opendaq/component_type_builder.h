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
#include <coreobjects/property_object.h>
#include <opendaq/component_type.h>
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
 * [interfaceLibrary(IComponentType, "opendaq")]
 */

/*!
 * @brief Builder component of Component type objects. Contains setter methods to configure the Component type parameters, and a
 * `build` method that builds the object.
 *
 * Depending on the set "Type" builder parameter, a different Component type is created - eg. Streaming type,
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
    /*!
     * @brief Sets the unique component type id.
     * @param id The unique id of a component type.
     *
     * Unique id should not be presented on the UI.
     */
    virtual ErrCode INTERFACE_FUNC setId(IString* id) = 0;

    /*!
     * @brief Gets the unique component type id.
     * @param[out] id The unique id of a component type.
     *
     * Unique id should not be presented on the UI.
     */
    virtual ErrCode INTERFACE_FUNC getId(IString** id) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the component type sort. Can be either Streaming, Function block, Device, or Server.
     * Depending on the setting, the corresponding Component type object will be built.
     * @param sort The sort of the component type.
     */
    virtual ErrCode INTERFACE_FUNC setTypeSort(ComponentTypeSort sort) = 0;

    /*!
     * @brief Gets the component type sort. Can be either Streaming, Function block, Device, or Server.
     * Depending on the setting, the corresponding Component type object will be built.
     * @param sort The sort of the component type.
     */
    virtual ErrCode INTERFACE_FUNC getTypeSort(ComponentTypeSort* sort) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the user-friendly name of a component type.
     * @param name The user-friendly name of a component type.
     *
     * Name is usually presented on the UI. Does not have to be unique.
     */
    virtual ErrCode INTERFACE_FUNC setName(IString* name) = 0;

    /*!
     * @brief Gets the user-friendly name of a component type.
     * @param[out] name The user-friendly name of a component type.
     *
     * Name is usually presented on the UI. Does not have to be unique.
     */
    virtual ErrCode INTERFACE_FUNC getName(IString** name) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the description of a component type.
     * @param description The description of a component type.
     *
     * A short description of a component type and the associated configuration parameters it offers.
     */
    virtual ErrCode INTERFACE_FUNC setDescription(IString* description) = 0;

    /*!
     * @brief Gets the description of a component type.
     * @param[out] description The description of a component type.
     *
     * A short description of a component type and the associated configuration parameters it offers.
     */
    virtual ErrCode INTERFACE_FUNC getDescription(IString** description) = 0;

    // [returnSelf]
    /*
     * @brief Sets the prefix found in connection strings used to establish a streaming connection of
     * this type.
     * @param[out] prefix The connection string prefix.
     *
     * The prefix is always found at the start of the connection string, before the "://" delimiter.
     */
    virtual ErrCode INTERFACE_FUNC setConnectionStringPrefix(IString* prefix) = 0;

    /*
     * @brief Gets the prefix found in connection strings used to establish a streaming connection of
     * this type.
     * @param[out] prefix The connection string prefix.
     *
     * The prefix is always found at the start of the connection string, before the "://" delimiter.
     */
    virtual ErrCode INTERFACE_FUNC getConnectionStringPrefix(IString** prefix) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the default configuration object that will be cloned and passed to users
     * by the built Component type when requested.
     * @param defaultConfig The default configuration object.
     *
     * Configuration objects are property object with user-defined key-value pairs.
     * For example: Port=1000, OutputRate=5000, ...
     */
    virtual ErrCode INTERFACE_FUNC setDefaultConfig(IPropertyObject* defaultConfig) = 0;

    /*!
     * @brief Gets the default configuration object that will be cloned and passed to users
     * by the built Component type when requested.
     * @param[out] defaultConfig The default configuration object.
     *
     * Configuration objects are property object with user-defined key-value pairs.
     * For example: Port=1000, OutputRate=5000, ...
     */
    virtual ErrCode INTERFACE_FUNC getDefaultConfig(IPropertyObject** defaultConfig) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the module information.
     * @param info The module information.
     */
    virtual ErrCode INTERFACE_FUNC setModuleInfo(IModuleInfo* info) = 0;

    /*!
     * @brief Gets the module information.
     * @param[out] info The module information.
     */
    virtual ErrCode INTERFACE_FUNC getModuleInfo(IModuleInfo** info) = 0;
};
/*!@}*/

/*!
 * @ingroup objects_component_type
 * @addtogroup objects_component_type_factories Factories
 * @{
 */

/*!
 * @brief Creates a ComponentTypeBuilder with default parameters.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, ComponentTypeBuilder, IComponentTypeBuilder)

/*!
 * @brief Creates a ComponentTypeBuilder with the type sort set to "Device".
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, DeviceTypeBuilder, IComponentTypeBuilder)

/*!
 * @brief Creates a ComponentTypeBuilder with the type sort set to "Streaming".
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, StreamingTypeBuilder, IComponentTypeBuilder)

/*!
 * @brief Creates a ComponentTypeBuilder with the type sort set to "Server".
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, ServerTypeBuilder, IComponentTypeBuilder)

/*!
 * @brief Creates a ComponentTypeBuilder with the type sort set to "FunctionBlock".
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, FunctionBlockTypeBuilder, IComponentTypeBuilder)

/*!@}*/

END_NAMESPACE_OPENDAQ
