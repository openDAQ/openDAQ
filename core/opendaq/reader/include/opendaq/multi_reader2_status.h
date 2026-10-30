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
#include <coretypes/stringobject.h>
#include <opendaq/component.h>
#include <opendaq/data_descriptor.h>
#include <opendaq/multi_reader2_input_status.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_readers
 * @addtogroup opendaq_multi_reader2 Multi reader 2
 * @{
 */

/*!
 * @brief The immutable result of one read.
 *
 * The event fields, hasChanges, domainDescriptorChanged, resynchronized and each input's descriptorChanged, compare
 * this status with the previous one. A configure with new params, or a recovery, starts afresh, so its first status
 * sets them all. A status under Error gives no descriptors and sets none of them.
 */
DECLARE_OPENDAQ_INTERFACE(IMultiReader2Status, IBaseObject)
{
    /*!
     * @brief False when nothing differs from the previous status; the read's count then says it all.
     */
    virtual ErrCode INTERFACE_FUNC getHasChanges(Bool* hasChanges) = 0;

    /*!
     * @brief Gets the reader's level.
     *
     * Ok: every input is healthy. Warning: inputs are set aside under Exclude, the rest is read. Error: the reader is
     * invalid under its error policy, nothing is delivered and no descriptor is reported.
     */
    virtual ErrCode INTERFACE_FUNC getStatus(ComponentStatus* status) = 0;

    /*!
     * @brief Gets the status message: empty under Ok, never null; otherwise one entry per input in error, in slot
     * order.
     */
    virtual ErrCode INTERFACE_FUNC getStatusMessage(IString** message) = 0;

    /*!
     * @brief This status gives a main domain descriptor, and the previous status gave none or another one.
     */
    virtual ErrCode INTERFACE_FUNC getDomainDescriptorChanged(Bool* changed) = 0;
    virtual ErrCode INTERFACE_FUNC getDomainDescriptor(IDataDescriptor** descriptor) = 0;

    /*!
     * @brief Gets the main input in effect: the pinned one, or the reader's current choice.
     *
     * Null while the reader is invalid with an automatic main input, or has no inputs.
     */
    virtual ErrCode INTERFACE_FUNC getMainInput(IComponent** input) = 0;

    /*!
     * @brief The next packetOffset does not continue the last delivered sample.
     */
    virtual ErrCode INTERFACE_FUNC getResynchronized(Bool* resynchronized) = 0;

    /*!
     * @brief The reader waits for its inputs to reach a common start and delivers nothing until then.
     *
     * Not a change on its own.
     */
    virtual ErrCode INTERFACE_FUNC getSynchronizing(Bool* synchronizing) = 0;

    // [templateType(inputs, IMultiReader2InputStatus)]
    /*!
     * @brief Gets every input's status, in slot order.
     */
    virtual ErrCode INTERFACE_FUNC getInputs(IList** inputs) = 0;

    /*!
     * @brief Gets one input's status.
     * @retval OPENDAQ_ERR_INVALIDPARAMETER The input is not in the list.
     */
    virtual ErrCode INTERFACE_FUNC getInputStatus(IComponent* input, IMultiReader2InputStatus** status) = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
