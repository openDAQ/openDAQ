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
#include <coretypes/dictobject.h>
#include <coretypes/stringobject.h>
#include <opendaq/data_descriptor.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_readers
 * @addtogroup opendaq_multi_reader Multi reader
 * @{
 */

/*!
 * @brief The overall outcome of a multi reader operation.
 */
enum class MultiReader2StatusType : EnumType
{
    Data = 0,
    Event
};

/*!
 * @brief Machine readable per-input error causes.
 */
enum class MultiReader2InputError : EnumType
{
    SyncFailed = 0,
    DataLoss,
    Gap,
    InvalidDescriptor,
    InvalidDomain,
    Disconnected
};

/*!
 * @brief Status of a multi reader operation: outcome, descriptors, and per-input errors.
 */
DECLARE_OPENDAQ_INTERFACE(IMultiReader2Status, IBaseObject)
{
    /*!
     * @brief Gets the overall outcome: Data or Event; errors are reported per input via `getErrors`.
     * @param[out] status The outcome.
     */
    virtual ErrCode INTERFACE_FUNC getStatus(MultiReader2StatusType* status) = 0;

    /*!
     * @brief Gets the domain descriptor of the main input.
     * @param[out] descriptor The main input domain descriptor.
     */
    virtual ErrCode INTERFACE_FUNC getDomainDescriptor(IDataDescriptor** descriptor) = 0;

    // [templateType(descriptors, IString, IDataDescriptor)]
    /*!
     * @brief Gets the value descriptors of all inputs, keyed by input id.
     * @param[out] descriptors The dictionary of input id to value descriptor.
     */
    virtual ErrCode INTERFACE_FUNC getDescriptors(IDict** descriptors) = 0;

    // [templateType(dividers, IString, IInteger)]
    /*!
     * @brief Gets the sample rate dividers of all inputs relative to the common rate, keyed by input id.
     * @param[out] dividers The dictionary of input id to divider.
     */
    virtual ErrCode INTERFACE_FUNC getDividers(IDict** dividers) = 0;

    // [templateType(errors, IString, IInteger)]
    /*!
     * @brief Gets the machine readable errors of failing inputs, keyed by input id.
     * @param[out] errors The dictionary of input id to `MultiReader2InputError` value.
     */
    virtual ErrCode INTERFACE_FUNC getErrors(IDict** errors) = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
