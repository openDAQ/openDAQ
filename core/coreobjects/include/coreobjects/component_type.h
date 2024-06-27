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
#include <coretypes/stringobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [templated(defaultAliasName: ComponentTypePtr)]
 * [interfaceSmartPtr(IComponentType, GenericComponentTypePtr)]
 */

/*!
 * @ingroup objects_utility
 * @addtogroup objects_component_type Component type
 * @{
 */

/*!
 * @brief Provides information about the component types.
 *
 * Is a Struct core type, and has access to Struct methods internally. Note that the Default config is not part of
 * the Struct fields.
 */

DECLARE_OPENDAQ_INTERFACE(IComponentType, IBaseObject)
{
    /*!
     * @brief Gets the unique component type id.
     * @param[out] id The unique id of a component type.
     *
     * Unique id should not be presented on the UI.
     */
    virtual ErrCode INTERFACE_FUNC getId(IString** id) = 0;

    /*!
     * @brief Gets the user-friendly name of a component type.
     * @param[out] name The user-friendly name of a component type.
     *
     * Name is usually presented on the UI. Does not have to be unique.
     */
    virtual ErrCode INTERFACE_FUNC getName(IString** name) = 0;

    /*!
     * @brief Gets the description of a component type.
     * @param[out] description The description of a component type.
     *
     * A short description of a component type and the associated configuration parameters it offers.
     */
    virtual ErrCode INTERFACE_FUNC getDescription(IString** description) = 0;

    /*!
     * @brief The function clones and returns default configuration. On each call, we need to create new object,
     * because we want that each instance of the component has its own configuration object.
     * @param[out] defaultConfig Newly created configuration object.
     *
     * Configuration objects are property object with user-defined key-value pairs.
     * For example: Port=1000, OutputRate=5000, ...
     */
    virtual ErrCode INTERFACE_FUNC createDefaultConfig(IPropertyObject** defaultConfig) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
