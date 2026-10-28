/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
#include <opendaq/device_type.h>
#include <coretypes/stringobject.h>

BEGIN_NAMESPACE_OPENDAQ

struct IAuthenticationMethod;

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
 * [interfaceLibrary(IAuthenticationMethod, "opendaq")]
 * [interfaceSmartPtr(IAuthenticationMethod, AuthenticationMethodPtr, "<opendaq/authentication_method_ptr.h>")]
 * [interfaceLibrary(IDeviceType, "opendaq")]
 * [interfaceSmartPtr(IDeviceType, DeviceTypePtr, "<opendaq/device_type_ptr.h>")]
 */

/*!
 * @brief Builder component of Component type objects. Contains setter methods to configure the Component type parameters, and a
 * `build` method that builds the object.
 *
 * Depending on the set "Type" builder parameter, a different Component type is created - eg. Streaming type,
 * Device type, Function block type, or Server type.
 * `addAuthenticationMethod` only applies to a "Device" or "Streaming" sort - fails with
 * `OPENDAQ_ERR_NOT_SUPPORTED` for any other sort. If never called, the built type ends up with no supported
 * authentication methods - so the core treats it as not supporting authentication, going through the plain
 * `createDevice`/`createStreaming` module path instead of `createAuthenticatedDevice`/`createAuthenticatedStreaming`.
 * `setLinkedDeviceType` only applies to a "Streaming" sort - fails with `OPENDAQ_ERR_NOT_SUPPORTED` for any
 * other sort, "Device" included. It is also mutually exclusive with `setDefaultConfig`/`addAuthenticationMethod`
 * - whichever is called second fails with `OPENDAQ_ERR_INVALID_OPERATION` - since a linked device type
 * redirects the built Streaming type's default config and supported authentication methods to that device
 * type's own instead.
 */
DECLARE_OPENDAQ_INTERFACE(IComponentTypeBuilder, IBaseObject)
{
    /*!
     * @brief Builds and returns a Component type object using the currently set values of the Builder.
     * @param[out] componentType The built Component type.
     *
     * Depending on the set "sort" builder parameter, a different Component type is created - eg. Streaming type,
     * Device type, Function block type, or Server type.
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
     * Applies to the Device and Streaming sorts only; other sorts ignore it.
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
     * @retval OPENDAQ_ERR_INVALID_OPERATION if a device type was already linked via `setLinkedDeviceType` - the
     * built Streaming type's default config is redirected to that device type's own instead, so the two are
     * mutually exclusive.
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
     * @brief Sets the kind of device created by the built Device type.
     * @param kind The device kind.
     * @retval OPENDAQ_ERR_NOT_SUPPORTED for sorts other than Device.
     *
     * A builder that does not set the kind builds an External Device type.
     */
    virtual ErrCode INTERFACE_FUNC setDeviceKind(DeviceKind kind) = 0;

    /*!
     * @brief Gets the kind of device created by the built Device type.
     * @param[out] kind The device kind. External when not set.
     * @retval OPENDAQ_ERR_NOT_SUPPORTED for sorts other than Device.
     */
    virtual ErrCode INTERFACE_FUNC getDeviceKind(DeviceKind* kind) = 0;

    // [returnSelf]
    /*!
     * @brief Adds an authentication method the built Component type will support authenticating with,
     * appended after any already added. Kept in the order added, which a default selection follows -
     * selecting the first one added. Only valid when the "Type" builder parameter is "Device" or
     * "Streaming".
     * @param method The authentication method to add.
     * @retval OPENDAQ_ERR_ALREADYEXISTS for a second method with the same id.
     * @retval OPENDAQ_ERR_NOT_SUPPORTED for a Server or Function block sort.
     * @retval OPENDAQ_ERR_INVALID_OPERATION if a device type was already linked via `setLinkedDeviceType` - the
     * built Streaming type's supported authentication methods are redirected to that device type's own instead,
     * so the two are mutually exclusive.
     */
    virtual ErrCode INTERFACE_FUNC addAuthenticationMethod(IAuthenticationMethod* method) = 0;

    /*!
     * @brief Gets the authentication methods currently added to the builder - see `addAuthenticationMethod`
     * for when they take effect on the built Component type.
     * @param[out] authenticationMethods The supported authentication methods, keyed by their own id.
     */
    // [templateType(authenticationMethods, IString, IAuthenticationMethod)]
    virtual ErrCode INTERFACE_FUNC getSupportedAuthenticationMethods(IDict** authenticationMethods) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the device type the built Streaming type shares its connection parameters with (including
     * supported authentication methods). Only valid when the "Type" builder parameter is "Streaming".
     * @param deviceType The device type to link.
     * @retval OPENDAQ_ERR_NOT_SUPPORTED for any sort other than "Streaming".
     * @retval OPENDAQ_ERR_INVALID_OPERATION if a default config was already set via `setDefaultConfig`, or an
     * authentication method was already added via `addAuthenticationMethod` - a linked device type redirects
     * both instead, so it is mutually exclusive with either already being set.
     */
    virtual ErrCode INTERFACE_FUNC setLinkedDeviceType(IDeviceType* deviceType) = 0;

    /*!
     * @brief Gets the device type currently set on the builder - see `setLinkedDeviceType` for when it takes
     * effect on the built Streaming type.
     * @param[out] deviceType The linked device type, or unassigned if none was set.
     * @retval OPENDAQ_ERR_NOT_SUPPORTED for any sort other than "Streaming".
     */
    virtual ErrCode INTERFACE_FUNC getLinkedDeviceType(IDeviceType** deviceType) = 0;
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
