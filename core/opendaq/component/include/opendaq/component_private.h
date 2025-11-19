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

#include <coretypes/listobject.h>
#include <opendaq/component.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(ICoreEventArgs, "coreobjects")]
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 */

/*!
 * @brief Provides access to private methods of the component.
 *
 * Said methods allow for triggering a Core event of the component, and locking/unlocking attributes of
 * the component.
 */
DECLARE_OPENDAQ_INTERFACE(IComponentPrivate, IBaseObject)
{
    // [templateType(attributes, IString)]
    /*!
     * @brief Locks the attributes contained in the provided list.
     * @param attributes The list of attributes that should be locked. Is not case sensitive.
     */
    virtual ErrCode INTERFACE_FUNC lockAttributes(IList* attributes) = 0;

    /*!
     * @brief Locks all attributes of the component.
     */
    virtual ErrCode INTERFACE_FUNC lockAllAttributes() = 0;

    // [templateType(attributes, IString)]
    /*!
     * @brief Unlocks the attributes contained in the provided list.
     * @param attributes The list of attributes that should be unlocked. Is not case sensitive.
     */
    virtual ErrCode INTERFACE_FUNC unlockAttributes(IList* attributes) = 0;

    /*!
     * @brief Unlocks all attributes of the component.
     */
    virtual ErrCode INTERFACE_FUNC unlockAllAttributes() = 0;

    /*!
     * @brief Triggers the component-specific core event with the provided arguments.
     * @param args The arguments of the core event.
     */
    virtual ErrCode INTERFACE_FUNC triggerComponentCoreEvent(ICoreEventArgs* args) = 0;

    /*!
     * @brief Notifies component about the change of the operation mode.
     * @param modeType The new operation mode.
     */
    virtual ErrCode INTERFACE_FUNC updateOperationMode(OperationModeType modeType) = 0;

    /*!
     * @brief Sets the configuration which was used to create the component.
     * @param config The configuration of the component.
     */
    virtual ErrCode INTERFACE_FUNC setComponentConfig(IPropertyObject* config) = 0;

    /*!
     * @brief Retrieves the configuration which was used to create the component.
     * @param config The configuration of the component.
     */
    virtual ErrCode INTERFACE_FUNC getComponentConfig(IPropertyObject** config) = 0;

    /*!
     * @brief Called by parent component to notify the component it is active/inactive
     * @param parentActive True This/parent components is active.
     */
    virtual ErrCode INTERFACE_FUNC setParentActive(Bool parentActive) = 0;

    /*!
     * @brief Called by folder component to set it's active state
     * @param update Serializable object.
     */
    virtual ErrCode INTERFACE_FUNC updateActiveAttr(ISerializedObject * update) = 0;
};

END_NAMESPACE_OPENDAQ
