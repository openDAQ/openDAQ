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

#include <opendaq/context.h>
#include <coreobjects/property_object.h>
#include <opendaq/tags.h>
#include <opendaq/component_status_container.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_components
 * @addtogroup opendaq_component Component
 * @{
 */

/*#
 * [includeHeader("<opendaq/removable.h>")]
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 * [interfaceSmartPtr(IPropertyObject, GenericPropertyObjectPtr, "<coreobjects/property_object_ptr.h>")]
 * [templated(defaultAliasName: ComponentPtr)]
 * [interfaceSmartPtr(IComponent, GenericComponentPtr)]
 * [interfaceLibrary(ICoreEventArgs, "coreobjects")]
 * [includeHeader("<coretypes/event_wrapper.h>")]
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
     * @brief Sets the component to be either active or inactive. Also recursively sets the `active` field
     * of all child components if component is a folder.
     * @param active The new active state of the component.
     * @retval OPENDAQ_IGNORED if "Active" is part of the component's list of locked attributes,
     * or if the new active value is equal to the previous.
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
     * @retval OPENDAQ_IGNORED if "Name" is part of the component's list of locked attributes,
     * or if the new name value is equal to the previous.
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
     * @retval OPENDAQ_IGNORED if "Description" is part of the component's list of locked attributes,
     * or if the new description value is equal to the previous.
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
    virtual ErrCode INTERFACE_FUNC getTags(ITags** tags) = 0;

    /*!
     * @brief Gets `visible` metadata state of the component
     * @param[out] visible True if the component is visible; False otherwise.
     *
     * Visible determines whether search/getter methods return find the component by default.
     */
    virtual ErrCode INTERFACE_FUNC getVisible(Bool* visible) = 0;
    
    /*!
     * @brief Sets `visible` attribute state of the component
     * @param visible True if the component is visible; False otherwise.
     * @retval OPENDAQ_IGNORED if "Visible" is part of the component's list of locked attributes.
     *
     * Visible determines whether search/getter methods return find the component by default.
     */
    virtual ErrCode INTERFACE_FUNC setVisible(Bool visible) = 0;

    // [templateType(attributes, IString)]
    /*!
     * @brief Gets a list of the component's locked attributes. The locked attributes cannot be modified via their respective setters.
     * @param[out] attributes A list of strings containing the names of locked attributes in capital case (eg. "Name", "Description"). 
     */
    virtual ErrCode INTERFACE_FUNC getLockedAttributes(IList** attributes) = 0;
    
    // [templateType(event, IComponent, ICoreEventArgs)]
    /*!
     * @brief Gets the Core Event object that triggers whenever a change to this component happens within the openDAQ core structure.
     * @param[out] event The Core Event object. The event triggers with a Component reference and a CoreEventArgs object as arguments.
     *
     * The Core Event is triggered on various changes to the openDAQ Components. This includes changes to property values,
     * addition/removal of child components, connecting signals to input ports and others. The event type can be identified
     * via the event ID available within the CoreEventArgs object. Each event type has a set of predetermined parameters
     * available in the `parameters` field of the arguments. These can be used by any openDAQ server, or other listener to
     * react to changes within the core structure.
     */
    virtual ErrCode INTERFACE_FUNC getOnComponentCoreEvent(IEvent** event) = 0;

    /*!
     * @brief Gets the container of Component statuses.
     * @param[out] statusContainer The container of Component statuses.
     */
    virtual ErrCode INTERFACE_FUNC getStatusContainer(IComponentStatusContainer** statusContainer) = 0;  
  
    /*!
     * @brief Finds the component (signal/device/function block) with the specified (global) id.
     * @param id The id of the component to search for.
     * @param[out] outComponent The resulting component.
     *
     * If the component parameter is true, the starting component is the root device.
     *
     * The id provided should be in relative form from the starting component. E.g., to find a signal in
     * the starting component, the id should be in the form of "dev/dev_id/ch/ch_id/sig/sig_id.
     */
    virtual ErrCode INTERFACE_FUNC findComponent(IString* id, IComponent** outComponent) = 0;
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

/*!@}*/

END_NAMESPACE_OPENDAQ
