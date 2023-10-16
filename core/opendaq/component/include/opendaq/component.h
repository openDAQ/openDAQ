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
     * @param[out] name The name of the component.
     *
     * The object that implements this interface defines how the name is specified.
     */
    virtual ErrCode INTERFACE_FUNC getName(IString** name) = 0;

    /*!
     * @brief Gets the tags of the component.
     * @param[out] tags The tags of the component.
     *
     * Tags are user definable labels that can be attached to the component.
     */
    virtual ErrCode INTERFACE_FUNC getTags(ITagsConfig** tags) = 0;
};
/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, Component,
    IContext*, context,
    IComponent*, parent,
    IString*, localId,
    IString*, className
)

END_NAMESPACE_OPENDAQ
