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
 * @addtogroup opendaq_multi_reader Multi reader
 * @{
 */

/*!
 * @brief Why an input is in error; the consumer's reaction is the same for every kind.
 */
enum class MultiReader2InputError : EnumType
{
    None = 0,
    Disconnected,             ///< No signal connected to the port.
    ValueDescriptorInvalid,   ///< Not convertible to the read type, or declined by the owner's acceptsDescriptor.
    DomainDescriptorInvalid,  ///< Fails the input assumptions or mismatches the main input.
    SyncFailed                ///< Could not reach the common start: too far from the main input, or not before the deadline.
};

/*!
 * @brief One input's state on a status.
 */
DECLARE_OPENDAQ_INTERFACE(IMultiReader2InputStatus, IBaseObject)
{
    /*!
     * @brief Gets the signal or input port this entry describes.
     * @param[out] input The input component.
     */
    virtual ErrCode INTERFACE_FUNC getInput(IComponent** input) = 0;

    /*!
     * @brief Gets the input's error; None while healthy, persists while the cause holds. Under Invalidate an error
     * invalidates the reader; under Exclude the input is set aside.
     * @param[out] error The error kind.
     */
    virtual ErrCode INTERFACE_FUNC getError(MultiReader2InputError* error) = 0;

    /*!
     * @brief Gets whether the value descriptor is new since the previous status. Only while the reader is valid and
     * the input has no error. True on every healthy input on the first status after a configure or a recovery.
     * @param[out] changed True when the descriptor is new.
     */
    virtual ErrCode INTERFACE_FUNC getDescriptorChanged(Bool* changed) = 0;

    /*!
     * @brief Gets the input's value descriptor; null while the reader is invalid or the input is in error.
     * @param[out] descriptor The value descriptor.
     */
    virtual ErrCode INTERFACE_FUNC getDescriptor(IDataDescriptor** descriptor) = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
