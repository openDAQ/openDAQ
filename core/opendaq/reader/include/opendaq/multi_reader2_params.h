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
#include <opendaq/sample_type.h>

BEGIN_NAMESPACE_OPENDAQ

struct IComponent;

/*!
 * @ingroup opendaq_readers
 * @addtogroup opendaq_multi_reader Multi reader
 * @{
 */

/*!
 * @brief Configuration parameters applied to a multi reader via `configure`.
 */
DECLARE_OPENDAQ_INTERFACE(IMultiReader2Params, IBaseObject)
{
    /*!
     * @brief Gets the list of reader inputs (signals or input ports).
     * @param[out] inputs The list of inputs.
     */
    virtual ErrCode INTERFACE_FUNC getInputs(IList** inputs) = 0;

    /*!
     * @brief Sets the list of reader inputs; all items must be signals or all input ports.
     * @param inputs The list of inputs.
     */
    virtual ErrCode INTERFACE_FUNC setInputs(IList* inputs) = 0;

    /*!
     * @brief Gets the main input; the reader defaults to the first input when unset.
     * @param[out] input The main input, or null when unset.
     */
    virtual ErrCode INTERFACE_FUNC getMainInput(IComponent** input) = 0;

    /*!
     * @brief Sets the main input; it must also be present in the input list.
     * @param input The signal or input port acting as the main input.
     */
    virtual ErrCode INTERFACE_FUNC setMainInput(IComponent* input) = 0;

    // [templateType(inputs, IComponent)]
    /*!
     * @brief Gets the inputs that start out unused.
     * @param[out] inputs The list of unused inputs.
     */
    virtual ErrCode INTERFACE_FUNC getUnusedInputs(IList** inputs) = 0;

    // [templateType(inputs, IComponent)]
    /*!
     * @brief Sets the inputs that start out unused; each must be part of the input list and not the main input.
     * @param inputs The list of unused inputs.
     */
    virtual ErrCode INTERFACE_FUNC setUnusedInputs(IList* inputs) = 0;

    /*!
     * @brief Gets the read type value samples are converted to; mandatory, unset returns `OPENDAQ_ERR_NOTASSIGNED`.
     * @param[out] valueReadType The value read type.
     */
    virtual ErrCode INTERFACE_FUNC getValueReadType(SampleType* valueReadType) = 0;

    /*!
     * @brief Sets the read type value samples are converted to; mandatory.
     * @param valueReadType The value read type.
     */
    virtual ErrCode INTERFACE_FUNC setValueReadType(SampleType valueReadType) = 0;

    /*!
     * @brief Gets the minimum number of samples a read operation returns; defaults to 1.
     * @param[out] count The minimum read count.
     */
    virtual ErrCode INTERFACE_FUNC getMinReadCount(SizeT* count) = 0;

    /*!
     * @brief Sets the minimum number of samples a read operation returns; must be at least 1.
     * @param count The minimum read count.
     */
    virtual ErrCode INTERFACE_FUNC setMinReadCount(SizeT count) = 0;

    /*!
     * @brief Gets whether all inputs must share the same sample rate; defaults to False.
     * @param[out] requireSameRates True when equal rates are required.
     */
    virtual ErrCode INTERFACE_FUNC getRequireSameRates(Bool* requireSameRates) = 0;

    /*!
     * @brief Sets whether all inputs must share the same sample rate.
     * @param requireSameRates True when equal rates are required.
     */
    virtual ErrCode INTERFACE_FUNC setRequireSameRates(Bool requireSameRates) = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
