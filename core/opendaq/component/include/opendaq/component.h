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

#include <opendaq/context.h>
#include <coreobjects/property_object.h>
#include <opendaq/tags_config.h>

BEGIN_NAMESPACE_OPENDAQ

enum class ComponentStandardProps
{
    Add,
    AddReadOnly,
    Skip
};

/*#
 * [includeHeader("<opendaq/removable.h>")]
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, GenericPropertyObjectPtr, "<coreobjects/property_object_ptr.h>")]
 * [templated(defaultAliasName: ComponentPtr)]
 * [interfaceSmartPtr(IComponent, GenericComponentPtr)]
 */

/*!
 * @ingroup opendaq_components
 * @addtogroup opendaq_component Component
 * @{
 */

/*!
 * @brief Acts as a base interface for components, such as device, function block, channel and signal.
 *
 * The IComponent provides a set of methods that are common to all components:
 * LocalID, GlobalID and Active properties.
 */
DECLARE_OPENDAQ_INTERFACE(IComponent, IPropertyObject)
{
    /*!
     * @brief Gets the local ID of the component.
     * @param[out] localId The local ID of the component.
     *
     * Represents the identifier that is unique in a relation to the
     * parent component. There is no predefined format for local ID. It is a string defined
     * by its parent component.
     */
    virtual ErrCode INTERFACE_FUNC getLocalId(IString** localId) = 0;

    /*!
     * @brief Gets the global ID of the component.
     * @param[out] globalId The global ID of the component.
     *
     * Represents the identifier that is globally unique. Globally unique identifier is composed
     * from local identifiers from the parent components separated by '/' character. Device component
     * must make sure that its ID is globally unique.
     */
    virtual ErrCode INTERFACE_FUNC getGlobalId(IString** globalId) = 0;

    /*!
     * @brief Returns true if the component is active; false otherwise.
     * @param[out] active True if the component is active; false otherwise.
     *
     * An active component acquires data, performs calculations and send packets on the signal path.
     */
    virtual ErrCode INTERFACE_FUNC getActive(Bool* active) = 0;

    /*!
     * @brief Sets the component to be either active or inactive.
     * @param active The new active state of the component.
     *
     * An active component acquires data, performs calculations and send packets on the signal path.
     */
    virtual ErrCode INTERFACE_FUNC setActive(Bool active) = 0;

    /*!
     * @brief Gets the context object.
     * @param[out] context The context object.
     */
    virtual ErrCode INTERFACE_FUNC getContext(IContext** context) = 0;

    /*!
     * @brief Gets the parent of the component.
     * @param[out] parent The parent of the component.
     *
     * Every openDAQ component has a parent, except for instance. Parent should be passed as
     * a parameter to the constructor/factory. Once the component is created, the parent
     * cannot be changed.
     */
    virtual ErrCode INTERFACE_FUNC getParent(IComponent** parent) = 0;

    /*!
     * @brief Gets the name of the component.
     * @param[out] name The name of the component. Local ID if name is not configured.
     *
     * The object that implements this interface defines how the name is specified.
     */
    virtual ErrCode INTERFACE_FUNC getName(IString** name) = 0;

    /*!
     * @brief Sets the name of the component.
     * @param name The name of the component.
     * @retval OPENDAQ_IGNORED if the name is not configurable.
     * @retval OPENDAQ_ERR_ACCESSDENIED if the name is read-only.
     *
     * The object that implements this interface defines how the name is specified.
     */
    virtual ErrCode INTERFACE_FUNC setName(IString* name) = 0;

    /*!
     * @brief Gets the description of the component.
     * @param[out] description The description of the component. Empty if not configured.
     *
     * The object that implements this interface defines how the description is specified.
     */
    virtual ErrCode INTERFACE_FUNC getDescription(IString** description) = 0;

    /*!
     * @brief Sets the description of the component.
     * @param description The description of the component.
     * @retval OPENDAQ_IGNORED if the description is not configurable.
     * @retval OPENDAQ_ERR_ACCESSDENIED if the description is read-only.
     *
     * The object that implements this interface defines how the description is specified.
     */
    virtual ErrCode INTERFACE_FUNC setDescription(IString* description) = 0;

    /*!
     * @brief Gets the tags of the component.
     * @param[out] tags The tags of the component.
     *
     * Tags are user definable labels that can be attached to the component.
     */
    virtual ErrCode INTERFACE_FUNC getTags(ITagsConfig** tags) = 0;
};
/*!@}*/

/*!
 * @ingroup opendaq_component
 * @addtogroup opendaq_component_factories Factories
 * @{
 */

/*!
 * @brief Creates a component.
 * @param context The Context. Most often the creating function-block/device passes its own Context to the Folder.
 * @param parent The parent component.
 * @param localId The local ID of the component.
 */
OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, Component,
    IContext*, context,
    IComponent*, parent,
    IString*, localId,
    IString*, className
)

/*!
 * @brief Creates a component with a specific property mode for default properties.
 * @param context The Context. Most often the creating function-block/device passes its own Context to the Folder.
 * @param parent The parent component.
 * @param localId The local ID of the component.
 * @param propertyMode Integer property defining whether standard properties such as "Name" and "Description" are created.
 *                     0 to create the default properties; 1 to create the properties, but configure them as "read-only"; 2 to skip creation.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY,
    ComponentWithDefaultPropertyMode, IComponent,
    IContext*, context,
    IComponent*, parent,
    IString*, localId,
    IString*, className,
    Int, propertyMode
)

/*!@}*/

END_NAMESPACE_OPENDAQ
