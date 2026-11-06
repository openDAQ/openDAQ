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
 *
 * The reader keeps no clock: its limits are counted in each input's own data. An input with nothing queued while the
 * others have sent more than 3 s past its newest sample is reported as not synchronized by the next read, and it rejoins
 * once its own samples come within 1 s of the others'. An input whose first sample lies more than 3 s past the main
 * input's newest is reported the same way. A consumer that reads too slowly loses data: once an input queues more than
 * 5 s of its data or 100 MB, or all inputs together more than 1 GB, every input's queued data is dropped, and the next
 * status reports the loss.
 */
DECLARE_OPENDAQ_INTERFACE(IMultiReader2, IBaseObject)
{
    /*!
     * @brief Validates, then applies the params as if the reader were newly constructed: the queued data goes,
     * every input keeps what it last announced, and a 0-count read follows, whose status is returned. A rejected
     * object leaves the reader untouched. Equal params are applied the same way; to judge the inputs again without
     * reinitializing, use reevaluate. Schedules no onDataAvailable. Legal from inside the onDataAvailable handler.
     * @param params The parameters to apply.
     * @param[out] status The status of the 0-count read.
     */
    virtual ErrCode INTERFACE_FUNC configure(IMultiReader2Params* params, IMultiReader2Status** status) = 0;

    /*!
     * @brief Gets the aligned samples a read can return now: those every healthy input has in front of its first
     * queued event. 0 while synchronizing, while a status with changes is owed (the first read after construction
     * included), or when fewer than MinReadCount.
     * @param[out] count The available sample count.
     */
    virtual ErrCode INTERFACE_FUNC getAvailableCount(SizeT* count) = 0;

    // [arrayArg(data, count), arrayArg(domain, count), arrayArg(count, 1)]
    /*!
     * @brief Copies up to `count` aligned samples into one buffer per input, in slot order. The samples belong to
     * the descriptors reported before this status. `data` is typed `void*` because the binding generator cannot
     * express a jagged array. The buffer of an input in error is not written and may be null.
     * @param[in] data One buffer per input; null with `count` 0 is a status probe.
     * @param[in] domain Buffer for the main input's domain ticks, packetOffset plus the main rule's start (type: Int64,
     * size: count); when null, ticks are not returned (recommended).
     * @param[in,out] count In: the requested count, 0 or at least MinReadCount; out: the count read.
     * @param timeoutMs Placeholder for reading with timeout; a read never waits, and a non-zero value is NOTIMPLEMENTED.
     * @param[out] packetOffset The main tick of the first sample, relative to the main domain rule start.
     * @param[out] status The status of this read.
     */
    virtual ErrCode INTERFACE_FUNC read(void* data, void* domain, SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status) = 0;

    // [arrayArg(count, 1)]
    /*!
     * @brief Behaves exactly like read, without copying into buffers.
     * @param[in,out] count In: the requested count; out: the count skipped.
     * @param timeoutMs As on read.
     * @param[out] packetOffset The main tick of the first skipped sample, as on read.
     * @param[out] status The status of this read.
     */
    virtual ErrCode INTERFACE_FUNC skipSamples(SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status) = 0;

    // [templateType(event, IBaseObject, IEventArgs)]
    /*!
     * @brief Gets the event scheduled when data becomes readable or a status with changes awaits a read, also while
     * the reader is invalid. The sender is null. A call announces what arrived before it: the handler reads everything
     * available, as what it leaves is announced again only when more data or a change arrives.
     * @param[out] event The data available event.
     */
    virtual ErrCode INTERFACE_FUNC getOnDataAvailable(IEvent** event) = 0;

    /*!
     * @brief Judges every input again by acceptsDescriptor with the descriptors in force, without reinitializing, then
     * a 0-count read follows, whose status is returned. For a change of the owner's state that acceptsDescriptor
     * depends on. An input whose verdict changes is handled as on a descriptor change; the others keep their queued
     * data. Schedules no onDataAvailable. Legal from inside the onDataAvailable handler.
     * @param[out] status The status of the 0-count read.
     */
    virtual ErrCode INTERFACE_FUNC reevaluate(IMultiReader2Status** status) = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, MultiReader2, IMultiReader2, IMultiReader2Params*, params)

END_NAMESPACE_OPENDAQ
