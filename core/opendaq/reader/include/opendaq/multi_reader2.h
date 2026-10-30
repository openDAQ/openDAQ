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
 * @addtogroup opendaq_multi_reader2 Multi reader 2
 * @{
 */

/*#
 * [interfaceSmartPtr(IEventArgs, EventArgsPtr<>)]
 */

/*!
 * @brief Reads several signals at once, aligned onto the main input's tick grid.
 *
 * A sample's position is given in two forms. `packetOffset` is its tick from the main rule's start, the value a packet
 * carries as its offset; the ticks `readWithDomain` writes are counted from the main origin, so they are the
 * packetOffset plus the main rule's start.
 *
 * Synchronization and the lateness checks compare each input with the main input's data ingested so far, so where
 * the inputs' streams do not line up, which samples a read delivers can depend on the order in which the inputs'
 * packets arrive. The reader keeps no clock: a main input that stops sending is not reported, and the reader stays
 * valid and delivers nothing until the main input sends again.
 */
DECLARE_OPENDAQ_INTERFACE(IMultiReader2, IBaseObject)
{
    /*!
     * @brief Validates the params, then applies them as if the reader were newly constructed.
     * @param params The new setup. A rejected object leaves the reader untouched.
     * @param[out] status The status of the 0-count read that follows.
     *
     * The queued data is dropped and every input keeps what it last announced. Equal params skip the
     * reinitialization: every input is judged again by `acceptsDescriptor`, then the read follows. Legal from inside
     * the `onDataAvailable` handler. A port the new params leave out is released before the call returns: remove such a
     * port from its owner after the call, never before, or the reader reports it as a disconnect first.
     */
    virtual ErrCode INTERFACE_FUNC configure(IMultiReader2Params* params, IMultiReader2Status** status) = 0;

    /*!
     * @brief Gets the count the next read returns in full.
     * @param[out] count The aligned samples in front of the first queued event, or 0 when that read returns none.
     */
    virtual ErrCode INTERFACE_FUNC getAvailableCount(SizeT* count) = 0;

    // [arrayArg(data, count), arrayArg(count, 1)]
    /*!
     * @brief Copies up to `count` aligned samples into one buffer per input, in slot order.
     * @param data One buffer per input; a buffer holds `count` samples of the input's sample size on the status.
     * @param[in,out] count The samples requested; out, the samples read. 0 with null data is a status probe.
     * @param timeoutMs Must be 0: a read never waits, and any other value returns NOTIMPLEMENTED. Reserved for
     * reading with a timeout.
     * @param[out] packetOffset The first sample's position on the main input, in the form the interface describes.
     * @param[out] status The status the samples belong to: they carry the descriptors reported before it.
     * @retval OPENDAQ_ERR_INVALIDPARAMETER A count below MinReadCount, or a null buffer for a contributing input.
     * @retval OPENDAQ_ERR_ARGUMENT_NULL A null count or status.
     */
    virtual ErrCode INTERFACE_FUNC read(void* data, SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status) = 0;

    // [arrayArg(data, count), arrayArg(count, 1)]
    /*!
     * @brief As `read`, with one extra leading buffer of Int64 main ticks counted from the main origin.
     */
    virtual ErrCode INTERFACE_FUNC readWithDomain(void* data, SizeT* count, SizeT timeoutMs, IMultiReader2Status** status) = 0;

    // [arrayArg(count, 1)]
    /*!
     * @brief Behaves exactly like `read`, without copying into buffers.
     * @param[in,out] count The samples requested; out, the samples skipped.
     * @param[out] packetOffset The position of the first skipped sample on the main input.
     */
    virtual ErrCode INTERFACE_FUNC skipSamples(SizeT* count, SizeT timeoutMs, SizeT* packetOffset, IMultiReader2Status** status) = 0;

    // [templateType(event, IBaseObject, IEventArgs)]
    /*!
     * @brief Gets the event called when data becomes readable or a status with changes awaits a read.
     *
     * Called also while the reader is invalid; the sender is null. Calls never overlap. After a call returns,
     * another is scheduled while data stays readable, so a handler reads what is available instead of waiting for
     * more. A handler subscribed after the last trigger is first called on the next one: a consumer that subscribes
     * after data has arrived reads once itself.
     */
    virtual ErrCode INTERFACE_FUNC getOnDataAvailable(IEvent** event) = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, MultiReader2, IMultiReader2, IMultiReader2Params*, params)

END_NAMESPACE_OPENDAQ
