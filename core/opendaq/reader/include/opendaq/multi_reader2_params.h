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
#include <coretypes/listobject.h>
#include <opendaq/component.h>
#include <opendaq/sample_type.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_readers
 * @addtogroup opendaq_multi_reader Multi reader
 * @{
 */

/*!
 * @brief What an erroring used input does to the reader.
 */
enum class MultiReader2ErrorPolicy : EnumType
{
    Invalidate = 0,  ///< The reader is invalid until every used input is healthy.
    Exclude          ///< The input is set aside and rejoins when healthy; invalid only when no used input is healthy.
};

/*!
 * @brief Parameters applied to a multi reader via `configure`. Read at configure time and not retained.
 */
DECLARE_OPENDAQ_INTERFACE(IMultiReader2Params, IBaseObject)
{
    // [templateType(inputs, IComponent)]
    /*!
     * @brief Gets the inputs in slot order.
     * @param[out] inputs The list of signals or input ports.
     */
    virtual ErrCode INTERFACE_FUNC getInputs(IList** inputs) = 0;

    // [templateType(inputs, IComponent)]
    /*!
     * @brief Sets the inputs in slot order; non-empty, no duplicate ids, all signals or all input ports.
     * Used flags of inputs that stay in the list are kept.
     * @param inputs The list of signals or input ports.
     */
    virtual ErrCode INTERFACE_FUNC setInputs(IList* inputs) = 0;

    /*!
     * @brief Gets what everything else aligns against. Unset: the reader chooses the first used input without an
     * error and re-chooses when it errors. Set: pinned; it must be in the list and used, and its error invalidates
     * the reader.
     * @param[out] input The main input, or null when unset.
     */
    virtual ErrCode INTERFACE_FUNC getMainInput(IComponent** input) = 0;

    /*!
     * @brief Sets the main input; null clears the choice.
     * @param input The signal or input port everything else aligns against.
     */
    virtual ErrCode INTERFACE_FUNC setMainInput(IComponent* input) = 0;

    /*!
     * @brief Gets whether an input is used; true by default. An unused input keeps its buffer slot, delivers no
     * data, cannot invalidate the reader, and its state is reported.
     * @param input An input from the list; INVALIDPARAMETER otherwise.
     * @param[out] used True when the input takes part in reading.
     */
    virtual ErrCode INTERFACE_FUNC getInputUsed(IComponent* input, Bool* used) = 0;

    /*!
     * @brief Sets whether an input is used. A pinned main input cannot be unused.
     * @param input An input from the list; INVALIDPARAMETER otherwise.
     * @param used True when the input takes part in reading.
     */
    virtual ErrCode INTERFACE_FUNC setInputUsed(IComponent* input, Bool used) = 0;

    /*!
     * @brief Gets whether the reader reads at all; true by default. False behaves as if every input were unused.
     * @param[out] used True when the reader reads.
     */
    virtual ErrCode INTERFACE_FUNC getUsed(Bool* used) = 0;

    /*!
     * @brief Sets whether the reader reads at all.
     * @param used True when the reader reads.
     */
    virtual ErrCode INTERFACE_FUNC setUsed(Bool used) = 0;

    /*!
     * @brief Gets the type every input converts to; Float64 by default. An input that cannot convert is
     * ValueDescriptorInvalid. Undefined reads every input as-is in its own sample type.
     * @param[out] valueReadType The value read type.
     */
    virtual ErrCode INTERFACE_FUNC getValueReadType(SampleType* valueReadType) = 0;

    /*!
     * @brief Sets the type every input converts to.
     * @param valueReadType A numeric sample type, or Undefined to read every input as-is.
     */
    virtual ErrCode INTERFACE_FUNC setValueReadType(SampleType valueReadType) = 0;

    /*!
     * @brief Gets the smallest number of samples a read returns; at least 1, defaults to 1.
     * @param[out] count The minimum read count.
     */
    virtual ErrCode INTERFACE_FUNC getMinReadCount(SizeT* count) = 0;

    /*!
     * @brief Sets the smallest number of samples a read returns.
     * @param count The minimum read count; at least 1.
     */
    virtual ErrCode INTERFACE_FUNC setMinReadCount(SizeT count) = 0;

    /*!
     * @brief Gets what an erroring used input does to the reader; Invalidate by default. Exclude lets the reader set
     * erroring inputs aside and take them back on its own; a pinned main input is never set aside, its error
     * invalidates the reader.
     * @param[out] policy The error policy.
     */
    virtual ErrCode INTERFACE_FUNC getErrorPolicy(MultiReader2ErrorPolicy* policy) = 0;

    /*!
     * @brief Sets what an erroring used input does to the reader.
     * @param policy The error policy.
     */
    virtual ErrCode INTERFACE_FUNC setErrorPolicy(MultiReader2ErrorPolicy policy) = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, MultiReader2Params, IMultiReader2Params)

END_NAMESPACE_OPENDAQ
