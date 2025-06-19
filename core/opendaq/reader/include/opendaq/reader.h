/*
 * Copyright 2022-2025 openDAQ d.o.o.
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
#include <coretypes/procedure.h>
#include <opendaq/input_port_notifications.h>

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
     * @brief Sets the specified callback function to be called when there is available data in the reader.
     * @param callback The callback function to be set or @c nullptr to unset it.
     *
     * Pass @c nullptr to unset the callback. The callback should take no arguments.
     */
    virtual ErrCode INTERFACE_FUNC setOnDataAvailable(IProcedure* callback) = 0;

    /*!
     * @brief Sets an external listener to the reader.
     * @param listener The external listener.
     *
     * When an external listener is set, after the reader is done processing the input port notification methods,
     * it also calls the methods of the external listener.
     */
    virtual ErrCode INTERFACE_FUNC setExternalListener(IInputPortNotifications* listener) = 0;

    /*!
     * @brief Checks if there is data to read. 
     * @param[out] empty Set to true if there is data to read, false otherwise.
     * 
     * This method returns true if there is data available to read. 
     * The data can be an event packet or a sufficient amount of data samples for minimum reading.
     */
    virtual ErrCode INTERFACE_FUNC getEmpty(Bool* empty) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
