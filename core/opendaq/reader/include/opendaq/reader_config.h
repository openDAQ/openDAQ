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
#include <opendaq/reader.h>
#include <opendaq/input_port_config.h>
#include <opendaq/sample_type.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_readers
 * @addtogroup opendaq_reader_config Reader config
 * @{
 */

/*#
 * [include(IReader)]
 */
/*!
 * @brief An interface providing access to a new reader in order to reuse the invalidated reader's settings and configuration.
 */
DECLARE_OPENDAQ_INTERFACE(IReaderConfig, IBaseObject)
{
    /*!
     * @brief Gets the currently set callback to call when the signal descriptor changes if any.
     * @param[out] callback The callback to call when the descriptor changes or @c nullptr if not set.
     */
    virtual ErrCode INTERFACE_FUNC getOnDescriptorChanged(IFunction** callback) = 0;

    /*!
     * @brief Gets the transform function that will be called with the read value-data and currently valid Signal-Descriptor
     * giving the user the chance add a custom post-processing step.
     * @param[out] transform The function performing the post-processing or @c nullptr if not assigned.
     */
    virtual ErrCode INTERFACE_FUNC getValueTransformFunction(IFunction** transform) = 0;

    /*!
     * @brief Gets the transform function that will be called with the read domain-data and currently valid Signal-Descriptor
     * giving the user the chance add a custom post-processing step.
     * @param[out] transform The function performing the post-processing or @c nullptr if not assigned.
     */
    virtual ErrCode INTERFACE_FUNC getDomainTransformFunction(IFunction** transform) = 0;

    // [templateType(ports, IInputPortConfig)]
    /*!
     * @brief Gets the internally created input-ports if used.
     * @param[out] ports The internal Input-Ports if used by the reader otherwise @c nullptr.
     */
    virtual ErrCode INTERFACE_FUNC getInputPorts(IList** ports) = 0;

    /*!
     * @brief Gets the type of time-out handling used by the reader.
     * @param[out] timeoutType How the reader handles time-outs.
     */
    virtual ErrCode INTERFACE_FUNC getReadTimeoutType(ReadTimeoutType* timeoutType) = 0;

    /*!
     * @brief Marks the current reader as invalid preventing any additional operations to be performed on the reader
     * except reusing its info and configuration in a new reader.
     */
    virtual ErrCode INTERFACE_FUNC markAsInvalid() = 0;
};

END_NAMESPACE_OPENDAQ
