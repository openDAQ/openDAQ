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
#include <coretypes/stringobject.h>
#include <opendaq/input_port.h>
#include <opendaq/multi_reader2_params.h>
#include <opendaq/multi_reader2_status.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_readers
 * @addtogroup opendaq_multi_reader Multi reader
 * @{
 */

/*#
 * [interfaceSmartPtr(IEventArgs, EventArgsPtr<>)]
 */

/*!
 * @brief Reads several equal-rate signals at once, aligned onto the main input's tick grid: row i of every buffer
 * is the same tick. Everything the user does goes in through `configure` with a params object, and everything
 * the reader has to say comes back on `read`, as one status per read.
 */
DECLARE_OPENDAQ_INTERFACE(IMultiReader2, IBaseObject)
{
    /*!
     * @brief Applies the params wholesale: rebuilds the input list, discards all queued data, replays descriptors,
     * re-evaluates errors, starts synchronization if valid. Validates first, so a rejected object returns an error
     * and leaves the reader untouched. Params equal to the current configuration are a no-op. Legal from inside
     * the onDataAvailable handler.
     * @param params The parameters to apply.
     */
    virtual ErrCode INTERFACE_FUNC configure(IMultiReader2Params* params) = 0;

    /*!
     * @brief Gets the global id of the main input in effect: the pinned one, or the reader's current choice. Empty
     * when no used input is healthy.
     * @param[out] inputId The global id of the main input.
     */
    virtual ErrCode INTERFACE_FUNC getMainInput(IString** inputId) = 0;

    /*!
     * @brief Gets the number of aligned samples readable right now: the run in front of the earliest queued
     * boundary. 0 unless the reader is streaming and the run reaches MinReadCount.
     * @param[out] count The available sample count.
     */
    virtual ErrCode INTERFACE_FUNC getAvailableCount(SizeT* count) = 0;

    // [arrayArg(data, count), arrayArg(count, 1)]
    /*!
     * @brief Copies at most `count` aligned samples per input. `data` is a jagged array, one buffer per input in
     * slot order, each `count` samples wide in ValueReadType (or the input's own type when Undefined), typed
     * `void*` because the binding generator cannot express jagged in-arrays. The buffer of an unused or erroring
     * input is not written and may be null. Data and changes may come in one read: the data belongs to the
     * descriptors reported before, the changes apply from the next read on.
     * @param[in] data One buffer per input; null with `count` 0 is the status probe.
     * @param[in,out] count In: the requested count, 0 or at least MinReadCount; out: the count read.
     * @param[out] packetOffset The main-tick offset of the first sample, relative to the main domain rule start.
     * @param[out] status The status of this read; always assigned.
     */
    virtual ErrCode INTERFACE_FUNC read(void* data, SizeT* count, SizeT* packetOffset, IMultiReader2Status** status) = 0;

    // [arrayArg(data, count), arrayArg(count, 1)]
    /*!
     * @brief Same as `read` with one extra leading buffer taking Int64 main-tick timestamps.
     * @param[in] data Input count + 1 buffers; the first receives the timestamps.
     * @param[in,out] count In: the requested count; out: the count read.
     * @param[out] status The status of this read; always assigned.
     */
    virtual ErrCode INTERFACE_FUNC readWithDomain(void* data, SizeT* count, IMultiReader2Status** status) = 0;

    // [arrayArg(count, 1)]
    /*!
     * @brief Discards at most `count` aligned samples without copying, under the same boundary and MinReadCount
     * contract as `read`.
     * @param[in,out] count In: the requested count; out: the count skipped.
     * @param[out] status The status of this read; always assigned.
     */
    virtual ErrCode INTERFACE_FUNC skipSamples(SizeT* count, IMultiReader2Status** status) = 0;

    // [templateType(event, IInputPort, IEventArgs)]
    /*!
     * @brief Gets the event scheduled when data becomes deliverable or a status with changes awaits a read, also
     * while the reader is invalid. The sender argument is not assigned.
     * @param[out] event The data available event.
     */
    virtual ErrCode INTERFACE_FUNC getOnDataAvailable(IEvent** event) = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, MultiReader2, IMultiReader2, IMultiReader2Params*, params)

END_NAMESPACE_OPENDAQ
