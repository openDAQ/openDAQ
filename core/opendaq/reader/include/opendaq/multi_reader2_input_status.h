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
#include <coretypes/baseobject.h>
#include <opendaq/component.h>
#include <opendaq/data_descriptor.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_readers
 * @addtogroup opendaq_multi_reader2 Multi reader 2
 * @{
 */

/*!
 * @brief Why an input is in error; the consumer's reaction is the same for every kind.
 */
enum class MultiReader2InputError : EnumType
{
    None = 0,
    Disconnected,             ///< No signal connected.
    ValueDescriptorInvalid,   ///< Missing, not convertible to the read type, or rejected by acceptsDescriptor.
    DomainDescriptorInvalid,  ///< Fails the domain constraints; the status message names the one that failed.
    SyncFailed                ///< Cannot be aligned with the main input: more than the sync limit from it, or its samples
                              ///< fall between its ticks; the status message names the cause.
};

/*!
 * @brief One input's state on a status.
 */
DECLARE_OPENDAQ_INTERFACE(IMultiReader2InputStatus, IBaseObject)
{
    /*!
     * @brief Gets the signal or input port.
     */
    virtual ErrCode INTERFACE_FUNC getInput(IComponent** input) = 0;

    /*!
     * @brief Gets the input's error: None while healthy; it persists while the cause holds.
     */
    virtual ErrCode INTERFACE_FUNC getError(MultiReader2InputError* error) = 0;

    /*!
     * @brief This status gives the input's descriptor, and the previous status gave none or another one.
     *
     * The first status after a configure with new params, or after a recovery, sets it on every healthy input.
     */
    virtual ErrCode INTERFACE_FUNC getDescriptorChanged(Bool* changed) = 0;
    virtual ErrCode INTERFACE_FUNC getDescriptor(IDataDescriptor** descriptor) = 0;

    /*!
     * @brief Gets the bytes one sample of this input takes in its read buffer, under the descriptor on this status.
     *
     * The element count (the product of the dimension sizes, 1 for a scalar) times the size of the read type, or of
     * the input's own sample type under Undefined. 0 for an input in error.
     */
    virtual ErrCode INTERFACE_FUNC getSampleSize(SizeT* size) = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
