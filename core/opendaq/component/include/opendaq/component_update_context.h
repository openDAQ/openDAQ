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
#include <coretypes/stringobject.h>
#include <coretypes/dictobject.h>
#include <opendaq/component.h>
#include <opendaq/signal.h>

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
    virtual ErrCode INTERFACE_FUNC getInputPortConnection(IString* parentId, IDict** connections) = 0;

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
};

/*!
 * @}
 */




END_NAMESPACE_OPENDAQ
