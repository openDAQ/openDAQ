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
#include <opendaq/multi_reader2_input_status.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_readers
 * @addtogroup opendaq_multi_reader Multi reader
 * @{
 */

/*!
 * @brief Immutable result of one read. It reports four things: value descriptor changes per input, the main domain
 * change, a resynchronization, and each input's error state.
 */
DECLARE_OPENDAQ_INTERFACE(IMultiReader2Status, IBaseObject)
{
    /*!
     * @brief Gets whether anything below differs from the previous status; false means count says it all.
     * @param[out] hasChanges True when something changed.
     */
    virtual ErrCode INTERFACE_FUNC getHasChanges(Bool* hasChanges) = 0;

    /*!
     * @brief Gets whether the reader is valid. False: the reader delivers nothing and reports no descriptors. Under
     * Invalidate, a used input is in error; under Exclude, no used input is healthy or the pinned main input is
     * in error.
     * @param[out] valid True when the reader delivers data.
     */
    virtual ErrCode INTERFACE_FUNC getValid(Bool* valid) = 0;

    /*!
     * @brief Gets whether the main domain descriptor is new since the previous status; only while valid. True on
     * the first status after a configure or a recovery.
     * @param[out] changed True when the domain descriptor is new.
     */
    virtual ErrCode INTERFACE_FUNC getDomainDescriptorChanged(Bool* changed) = 0;

    /*!
     * @brief Gets the main input's domain descriptor; null while the reader is invalid.
     * @param[out] descriptor The domain descriptor.
     */
    virtual ErrCode INTERFACE_FUNC getDomainDescriptor(IDataDescriptor** descriptor) = 0;

    /*!
     * @brief Gets whether synchronization restarted after a gap or a domain change (a configure and a recovery
     * included), so the next packetOffset is discontinuous; only while valid.
     * @param[out] resynchronized True when synchronization restarted.
     */
    virtual ErrCode INTERFACE_FUNC getResynchronized(Bool* resynchronized) = 0;

    // [templateType(inputs, IMultiReader2InputStatus)]
    /*!
     * @brief Gets every input in slot order, unused ones included.
     * @param[out] inputs The list of input statuses.
     */
    virtual ErrCode INTERFACE_FUNC getInputs(IList** inputs) = 0;

    /*!
     * @brief Gets one input's entry.
     * @param input An input of the reader; INVALIDPARAMETER otherwise.
     * @param[out] status The input's entry.
     */
    virtual ErrCode INTERFACE_FUNC getInputStatus(IComponent* input, IMultiReader2InputStatus** status) = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
