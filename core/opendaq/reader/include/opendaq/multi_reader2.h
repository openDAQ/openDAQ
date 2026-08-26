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
#include <coretypes/event.h>
#include <opendaq/multi_reader2_params.h>
#include <opendaq/multi_reader2_status.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_readers
 * @addtogroup opendaq_multi_reader Multi reader
 * @{
 */

/*!
 * @brief Reads multiple signals at once.
 */
DECLARE_OPENDAQ_INTERFACE(IMultiReader2, IBaseObject)
{
    /*!
     * @brief Applies the params: unused inputs are removed, new ones added, and slots follow the params list order.
     * @param params The parameters holding the desired input list.
     */
    virtual ErrCode INTERFACE_FUNC configure(IMultiReader2Params* params) = 0;

    /*!
     * @brief Gets the global id of the resolved main input (the explicit choice, or the first input).
     * @param[out] inputId The main input id.
     */
    virtual ErrCode INTERFACE_FUNC getMainInput(IString** inputId) = 0;

    /*!
     * @brief Gets the number of samples readable from every used input.
     * @param[out] count The available sample count.
     */
    virtual ErrCode INTERFACE_FUNC getAvailableCount(SizeT* count) = 0;

    // [arrayArg(data, count), arrayArg(count, 1)]
    /*!
     * @brief Copies at most `count` unread samples of each input into the data buffers.
     * While an event is pending, the same event is reported again and no data is returned until `commitEvent`.
     * @param[out] status The status of the read operation.
     * @param[in] data Jagged array of one buffer per input, each at least `count` samples long.
     * @param[in,out] count In: the requested sample count; out: the count actually read.
     * @param[out] packetOffset The domain offset of the first read sample.
     */
    virtual ErrCode INTERFACE_FUNC read(IMultiReader2Status** status, void** data, SizeT* count, SizeT* packetOffset) = 0;

    // [arrayArg(data, count), arrayArg(count, 1)]
    /*!
     * @brief Same as `read`, but the first buffer in `data` receives the timestamps.
     * @param[out] status The status of the read operation.
     * @param[in] data Jagged array of input count + 1 buffers; the first one is populated with timestamps.
     * @param[in,out] count In: the requested sample count; out: the count actually read.
     */
    virtual ErrCode INTERFACE_FUNC readWithDomain(IMultiReader2Status** status, void** data, SizeT* count) = 0;

    /*!
     * @brief Commits the pending event: staged `setUsed`/`setActive` changes apply and reading resumes.
     */
    virtual ErrCode INTERFACE_FUNC commitEvent() = 0;

    /*!
     * @brief Marks an input as used or unused; only valid between an event read and `commitEvent`.
     * @param inputId The global id of the input.
     * @param used True when the input takes part in reading.
     */
    virtual ErrCode INTERFACE_FUNC setUsed(IString* inputId, Bool used) = 0;

    /*!
     * @brief Activates or deactivates the reader; only valid between an event read and `commitEvent`.
     * @param active True to activate.
     */
    virtual ErrCode INTERFACE_FUNC setActive(Bool active) = 0;

    // [templateType(event, IInputPort, IEventArgs)]
    /*!
     * @brief Gets the event triggered when a signal is connected to one of the reader ports.
     * @param[out] event The connected event.
     */
    virtual ErrCode INTERFACE_FUNC getOnConnected(IEvent** event) = 0;

    // [templateType(event, IInputPort, IEventArgs)]
    /*!
     * @brief Gets the event triggered when a signal is disconnected from one of the reader ports.
     * @param[out] event The disconnected event.
     */
    virtual ErrCode INTERFACE_FUNC getOnDisconnected(IEvent** event) = 0;

    // [templateType(event, IInputPort, IEventArgs)]
    /*!
     * @brief Gets the event triggered when data is available on all used inputs; not triggered yet.
     * @param[out] event The data available event.
     */
    virtual ErrCode INTERFACE_FUNC getOnDataAvailable(IEvent** event) = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
