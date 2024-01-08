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
#include <coretypes/function.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_readers
 * @addtogroup opendaq_reader Reader
 * @{
 */

/*!
 * @brief Controls how Reader read call time-outs are handled.
 */
enum class ReadTimeoutType
{
    Any, /*!< When some segments are available return them immediately.
          *   When no segments are available return immediately when any arrive or time-out is exceeded.
          */
    All  /*!< Wait for the requested amount or until time-out is exceeded.*/
};

/*#
 * [templated(defaultAliasName: ReaderPtr)]
 * [interfaceSmartPtr(IReader, GenericReaderPtr)]
 */

/*!
 * @brief A basic signal reader that simplifies accessing the signals's data stream.
 */
DECLARE_OPENDAQ_INTERFACE(IReader, IBaseObject)
{
    /*!
     * @brief Gets the number of segments available to read
     * @param[out] count The number of available segments
     */
    virtual ErrCode INTERFACE_FUNC getAvailableCount(SizeT* count) = 0;

    /*!
     * @brief Gets the user the option to invalidate the reader when the signal descriptor changes.
     * @param callback The callback to call when the descriptor changes or @c nullptr to unset it.
     * The callback takes a value and domain Signal descriptors as a parameters and returns a boolean indicating
     * whether the change is still acceptable. In the case the value or domain descriptor did not change
     * it will be @c nullptr. So either of the descriptors can be @c nullptr but not both.
     *
     * If the callback is not assigned or is set to @c nullptr the reader will just check if the new sample-type
     * is still implicitly convertible to the read type and invalidate itself if that is not the case.
     */
    virtual ErrCode INTERFACE_FUNC setOnDescriptorChanged(IFunction* callback) = 0;

    /*!
     * @brief Set callback which will be triggered if reader recieves packets
     * @param callback The callback to call when there are avaiable packets in reader or @c nullptr to unset it.
     * The callback takes no arguments
     */
    virtual ErrCode INTERFACE_FUNC setOnDataAvailable(IFunction* callback) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
