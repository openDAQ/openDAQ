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
#include <opendaq/tail_reader.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [include(ISampleReader)]
 * [interfaceSmartPtr(ISampleReader, GenericSampleReaderPtr)]
 */

/*!
 * @ingroup opendaq_readers
 * @addtogroup opendaq_duration_tail_reader Duration tail reader
 * @{
 */

/*!
 * @brief A tail reader that keeps only the samples within a trailing time window, measured in
 * milliseconds from the domain value of the most recently received sample, instead of a fixed
 * sample count.
 */
DECLARE_OPENDAQ_INTERFACE(IDurationTailReader, ISampleReader)
{
     // [arrayArg(values, count), arrayArg(count, 1)]
    /*!
     * @brief Copies at maximum the next `count` unread samples to the values buffer.
     * The amount actually read is returned through the `count` parameter.
     * @param[in] values The buffer that the samples will be copied to.
     * The buffer must be a contiguous memory big enough to receive `count` amount of samples.
     * @param[in,out] count The maximum amount of samples to be read. If the `count` is less than
     * available the parameter value is set to the actual amount and only the available
     * samples are returned. The rest of the buffer is not modified or cleared.
     * @param[out] status: Represents the status of the reader.
     * - If the reader is invalid, IReaderStatus::getValid returns false.
     * - If an event packet was encountered during processing, IReaderStatus::getReadStatus returns ReadStatus::Event
     * - If the reading process is successful, IReaderStatus::getReadStatu returns ReadStatus::Ok, indicating that IReaderStatus::getValid is true and there is no encountered events
     */
    virtual ErrCode INTERFACE_FUNC read(void* values, SizeT* count, ITailReaderStatus** status = nullptr) = 0;

    // [arrayArg(values, count), arrayArg(domain, count), arrayArg(count, 1)]
    /*!
     * @brief Copies at maximum the next `count` unread samples and clock-stamps to the `values` and `stamps` buffers.
     * The amount actually read is returned through the `count` parameter.
     * @param[in] values The buffer that the data values will be copied to.
     * The buffer must be a contiguous memory big enough to receive `count` amount of samples.
     * @param[in] domain The buffer that the domain values will be copied to.
     * The buffer must be a contiguous memory big enough to receive `count` amount of clock-stamps.
     * @param[in,out] count The maximum amount of samples to be read. If the `count` is less than
     * available the parameter value is set to the actual amount and only the available
     * samples are returned. The rest of the buffer is not modified or cleared.
     * @param[out] status: Represents the status of the reader.
     * - If the reader is invalid, IReaderStatus::getValid returns false.
     * - If an event packet was encountered during processing, IReaderStatus::getReadStatus returns ReadStatus::Event
     * - If the reading process is successful, IReaderStatus::getReadStatu returns ReadStatus::Ok, indicating that IReaderStatus::getValid is true and there is no encountered events
     */
    virtual ErrCode INTERFACE_FUNC readWithDomain(void* values, void* domain, SizeT* count, ITailReaderStatus** status = nullptr) = 0;

    /*!
     * @brief Gets the trailing history duration, in milliseconds.
     * @param[out] milliseconds The history duration.
     */
    virtual ErrCode INTERFACE_FUNC getHistoryDurationMs(UInt* milliseconds) = 0;

    /*!
     * @brief Sets the trailing history duration, in milliseconds. Buffered packets older than
     * the new duration relative to the latest received sample are dropped immediately.
     * @param milliseconds The new history duration. Must be positive.
     */
    virtual ErrCode INTERFACE_FUNC setHistoryDurationMs(UInt milliseconds) = 0;
};
/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, DurationTailReader,
    ISignal*, signal,
    UInt, historyDurationMs,
    SampleType, valueReadType,
    SampleType, domainReadType,
    ReadMode, mode
)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, DurationTailReaderFromPort, IDurationTailReader,
    IInputPortConfig*, port,
    UInt, historyDurationMs,
    SampleType, valueReadType,
    SampleType, domainReadType,
    ReadMode, mode
)

END_NAMESPACE_OPENDAQ
