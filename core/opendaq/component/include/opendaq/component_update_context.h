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
#include <coretypes/stringobject.h>
#include <coretypes/dictobject.h>
#include <opendaq/component.h>
#include <opendaq/signal.h>
#include <opendaq/device_update_options.h>
#include <opendaq/update_parameters.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup types_utility
 * @defgroup types_updatable Updatable
 * @{
 */

DECLARE_OPENDAQ_INTERFACE(IComponentUpdateContext, IBaseObject)
{
    /*!
     * @brief Sets signal connection to the input port for the specified parent component which is usualy a function block.
     * @param parentId The ID of the parent component.
     * @param portId The ID of the input port.
     * @param signalId The ID of the signal.
     */
    virtual ErrCode INTERFACE_FUNC setInputPortConnection(IString* parentId, IString* portId, IString* signalId) = 0;

    // [templateType(connections, IString, IString)]
    /*!
     * @brief Gets the dictionary with key-value pairs of input port local IDs and signal IDs for the specified parent component.
     * @param parentId The ID of the parent component.
     * @param[out] connections The connections to the input ports.
     */
    virtual ErrCode INTERFACE_FUNC getInputPortConnections(IString* parentId, IDict** connections) = 0;

    /*!
     * @brief Removes the connection to the input port for the specified parent component.
     * @param parentId The ID of the parent component.
     */
    virtual ErrCode INTERFACE_FUNC removeInputPortConnection(IString* parentId) = 0;
    
    /*!
     * @brief Sets the root component of the current component.
     * @param rootComponent The root component.
     */
    virtual ErrCode INTERFACE_FUNC setRootComponent(IComponent* rootComponent) = 0;

    /*!
     * @brief Gets the root component of the current component.
     * @param[out] rootComponent The root component.
     */
    virtual ErrCode INTERFACE_FUNC getRootComponent(IComponent** rootComponent) = 0;

    /*!
     * @brief Gets the signal by the specified parent and port ID.
     * @param parentId The ID of the parent component.
     * @param portId The ID of the input port.
     * @param[out] signal The found signal. If signal is not found signal is set to nullptr.
     */
    virtual ErrCode INTERFACE_FUNC getSignal(IString* parentId, IString* portId, ISignal** signal) = 0;

    /*!
     * @brief Sets the signal dependency to the function block.
     * @param signalId The ID of the signal.
     * @param parentId The ID of the parent component.
     */
    virtual ErrCode INTERFACE_FUNC setSignalDependency(IString* signalId, IString* parentId) = 0;

    /*!
     * @brief Adds a device remapping from the original device's local ID to the new device local ID. 
     * @param originalDeviceId The local ID of the original device to be remapped.
     * @param newDeviceId The local ID of the new device to be used for remapping.
     * 
     * Used to remap signal -> input port connections to the remapped device when loading.
     */
    virtual ErrCode INTERFACE_FUNC addDeviceRemapping(IString* originalDeviceId, IString* newDeviceId) = 0;

    /*!
     * @brief Gets the DeviceUpdateOptions object for the device with the specified local ID. Returns null if no options are found for the device.
     * @param localId The local ID of the device to get the options for.
     * @param options The DeviceUpdateOptions object for the device with the specified local ID; null if no options are found for the device.
     */
    virtual ErrCode INTERFACE_FUNC getDeviceUpdateOptionsWithLocalIdOrNull(IString* localId, IDeviceUpdateOptions** options) = 0;

    /*!
     * @brief Internal method that uses the device mapping to remap the input port connections.
     *
     * Should be called after the initial update, but before `onUpdatableUpdateEnd`.
     */
    virtual ErrCode INTERFACE_FUNC remapInputPortConnections() = 0;

    /*!
     * @brief Gets the update parameters provided by the user through the `update` call.
     * @param updateParameters The update parameters.
     */
    virtual ErrCode INTERFACE_FUNC getUpdateParameters(IUpdateParameters** updateParameters) = 0;

    /*!
     * @brief Overrides the internal context state with that of another.
     * @param updateContext The context with which the object is to be overridden.
     */
    virtual ErrCode INTERFACE_FUNC overrideState(IComponentUpdateContext* updateContext) = 0; 
    
    // [templateType(state, IString, IBaseObject)]
    virtual ErrCode INTERFACE_FUNC getInternalState(IDict** state) = 0;
};

/*!
 * @}
 */

END_NAMESPACE_OPENDAQ
