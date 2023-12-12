/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <opendaq/sample_reader.h>
#include <opendaq/signal.h>

BEGIN_NAMESPACE_OPENDAQ

struct IInputPort;

/*!
 * @ingroup opendaq_readers
 * @addtogroup opendaq_multi_reader Multi reader
 * @{
 */

/*#
 * [interfaceSmartPtr(ISampleReader, GenericSampleReaderPtr)]
 */

/*!
 * @brief Reads multiple Signals at once.
 */
DECLARE_OPENDAQ_INTERFACE(IMultiReader, ISampleReader)
{
    // [arrayArg(samples, count), arrayArg(count, 1)]
    /*!
     * @brief Copies at maximum the next `count` unread samples to the values buffer.
     * The amount actually read is returned through the `count` parameter.
     * @param[in] samples The buffer that the samples will be copied to.
     * The buffer must be a contiguous memory big enough to receive `count` amount of samples.
     * This should be a jagged array (array of pointers to arrays) where the size is equal to
     * the Signal count and each Signal buffer is at least `count` size long.
     * E.g: reading the next 5 samples of 3 signals
     * samples
     *  |
     *  ˇ     0  1  2  3  4  5  <-- count
     * [0] = [0, 0, 0, 0, 0, 0]
     * [1] = [0, 0, 0, 0, 0, 0]
     * [2] = [0, 0, 0, 0, 0, 0]
     * @param[in,out] count The maximum amount of samples to be read. If the `count` is less than
     * available the parameter value is set to the actual amount and only the available
     * samples are returned. The rest of the buffer is not modified or cleared.
     * @param timeoutMs The maximum amount of time in milliseconds to wait for the requested amount of samples before returning.
     */
    virtual ErrCode INTERFACE_FUNC read(void* samples, SizeT* count, SizeT timeoutMs = 0) = 0;

    // [arrayArg(samples, count), arrayArg(domain, count), arrayArg(count, 1)]
    /*!
     * @brief Copies at maximum the next `count` unread samples and clock-stamps to the `samples` and `domain` buffers.
     * The amount actually read is returned through the `count` parameter.
     * @param[in] samples The buffer that the samples will be copied to.
     * The buffer must be a contiguous memory big enough to receive `count` amount of samples.
     * This should be a jagged array (array of pointers to arrays) where the size is equal to
     * the Signal count and each Signal buffer is at least `count` size long.
     * E.g: reading the next 5 samples of 3 signals
     * samples
     *  |
     *  ˇ     0  1  2  3  4  5  <-- count
     * [0] = [0, 0, 0, 0, 0, 0]
     * [1] = [0, 0, 0, 0, 0, 0]
     * [2] = [0, 0, 0, 0, 0, 0]
     * @param[in] domain The buffer that the domain values will be copied to.
     * The buffer must be a contiguous memory big enough to receive `count` amount of clock-stamps.
     * This should be a jagged array (array of pointers to arrays) where the size is equal to
     * the Signal count and each Signal buffer is at least `count` size long.
     * E.g: reading the next 5 samples of 3 signals
     * domain
     *  |
     *  ˇ     0  1  2  3  4  5  <-- count
     * [0] = [0, 0, 0, 0, 0, 0]
     * [1] = [0, 0, 0, 0, 0, 0]
     * [2] = [0, 0, 0, 0, 0, 0]
     * @param[in,out] count The maximum amount of samples to be read. If the `count` is less than
     * available the parameter value is set to the actual amount and only the available
     * samples are returned. The rest of the buffer is not modified or cleared.
     * @param timeoutMs The maximum amount of time in milliseconds to wait for the requested amount of samples before returning.
     */
    virtual ErrCode INTERFACE_FUNC readWithDomain(void* samples, void* domain, SizeT* count, SizeT timeoutMs = 0) = 0;

    /*!
     * @brief Gets the resolution the reader aligned all the signals to.
     * This is the highest resolution (lowest value) of all the signals to not loose the precision.
     * @param resolution The aligned resolution used for all read signals.
     */
    virtual ErrCode INTERFACE_FUNC getTickResolution(IRatio** resolution) = 0;

    /*!
     * @brief Gets the origin the reader aligned all the signals to.
     * This is usually the earliest (lowest value) from all the signals.
     * @param origin The origin all signals are aligned to.
     */
    virtual ErrCode INTERFACE_FUNC getOrigin(IString** origin) = 0;

    /*!
     * @brief Gets the domain value (offset) from the aligned origin at the point the reader starts to provide synchronized samples.
     * @param domainStart The domain point at which the reader managed to synchronize all the signals.
     */
    virtual ErrCode INTERFACE_FUNC getOffset(void* domainStart) = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, MultiReader, IMultiReader,
    IList*, signals,
    SampleType, valueReadType,
    SampleType, domainReadType,
    ReadMode, mode,
    ReadTimeoutType, timeoutType
)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, MultiReaderFromExisting, IMultiReader,
    IMultiReader*, invalidatedReader,
    SampleType, valueReadType,
    SampleType, domainReadType
)

END_NAMESPACE_OPENDAQ
