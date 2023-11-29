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
#include <coreobjects/property_object.h>
#include <opendaq/function_block_type.h>
#include <opendaq/signal.h>
#include <opendaq/folder.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IFolder, GenericFolderPtr, "<opendaq/folder_ptr.h>")]
 * [templated(defaultAliasName: FunctionBlockPtr)]
 * [interfaceSmartPtr(IFunctionBlock, GenericFunctionBlockPtr)]
 */

/*!
 * @ingroup opendaq_function_blocks
 * @addtogroup opendaq_function_block Function block
 * @{
 */

/*!
 * @brief Function blocks perform calculations on inputs/generate data, outputting new data in its
 * output signals as packets.
 *
 * Each function block describes its behaviour and identifiers in its FunctionBlockType structure. It
 * provides a list of input ports that can be connected to signals that the input port accepts, as well as a
 * list of output signals that carry the function block's output data.
 *
 * Additionally, as each function block is a property object, it can define its own set of properties, providing
 * user-configurable settings. In example, a FFT function block would expose a `blockSize` property, defining the
 * amount of samples to be used for calculation in each block.
 *
 * Function blocks also provide a status signal, through which a status packet is sent whenever a connection to a
 * new input port is formed, or when the status changes.
 */
DECLARE_OPENDAQ_INTERFACE(IFunctionBlock, IFolder)
{
    /*!
     * @brief Gets an information structure contain metadata of the function block type.
     * @param[out] type The Function block type object.
     */
    virtual ErrCode INTERFACE_FUNC getFunctionBlockType(IFunctionBlockType** type) = 0;

    // [elementType(ports, IInputPort)]
    /*!
     * @brief Gets a list of the function block's input ports.
     * @param searchFilter Provides an optional filter that filters out unwanted components and allows for recursion.
     * @param[out] ports The list of input ports.
     *
     * If searchFilter is not provided, the returned list contains only visible input ports and does not include those of
     * child function blocks.
     */
    virtual ErrCode INTERFACE_FUNC getInputPorts(IList** ports, ISearchFilter* searchFilter = nullptr) = 0;

    // [elementType(signals, ISignal)]
    /*!
     * @brief Gets the list of the function block's output signals.
     * @param searchFilter Provides an optional filter that filters out unwanted components and allows for recursion.
     * @param[out] signals The list of output signals.
     *
     * If searchFilter is not provided, the returned list contains only visible signals and does not include those of
     * child function blocks.
     */
    virtual ErrCode INTERFACE_FUNC getSignals(IList** signals, ISearchFilter* searchFilter = nullptr) = 0;

    // [elementType(signals, ISignal)]
    /*!
     * @brief Gets the list of the function block's visible output signals including signals from visible child function blocks.
     * @param searchFilter Provides an optional filter that filters out unwanted components and allows for recursion.
     * @param[out] signals The list of output signals.
     */
    virtual ErrCode INTERFACE_FUNC getSignalsRecursive(IList** signals, ISearchFilter* searchFilter = nullptr) = 0;

    /*!
     * @brief Gets the function block's status signal.
     * @param[out] statusSignal The status signal.
     *
     * The status signal sends out a status event packet every time it is connected to an input port.
     * Additionally, a status event packet is sent whenever the status of the function block changes.
     */
    virtual ErrCode INTERFACE_FUNC getStatusSignal(ISignal** statusSignal) = 0;

    // [elementType(functionBlocks, IFunctionBlock)]
    /*!
     * @brief Gets a list of sub-function blocks.
     * @param searchFilter Provides optional parameters such as "recursive" and "visibleOnly" to modify the search pattern.
     * @param[out] functionBlocks The list of sub-function blocks.
     *
     * If searchFilter is not provided, the returned list contains only visible function blocks and does not include those of
     * child function blocks.
     */
    virtual ErrCode INTERFACE_FUNC getFunctionBlocks(IList** functionBlocks, ISearchFilter* searchFilter = nullptr) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
