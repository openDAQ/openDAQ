/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <coretypes/common.h>
#include <coretypes/baseobject.h>
#include <opendaq/stream_reader.h>
#include <opendaq/input_port.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_readers
 * @addtogroup opendaq_stream_reader Stream reader builder
 * @{
 */

/*!
 * @brief Builder component of Stream reader objects. Contains setter methods to configure the Stream reader parameters
 * and a `build` method that builds the Unit object.
 */
DECLARE_OPENDAQ_INTERFACE(IStreamReaderBuilder, IBaseObject)
{
    /*!
     * @brief Builds and returns a Stream reader object using the currently set values of the Builder.
     * @param[out] streamReader The built Stream reader.
     */
    virtual ErrCode INTERFACE_FUNC build(IStreamReader** streamReader) = 0;

    // [returnSelf]
    /*!
     * @brief Sets old stream reader instance to copy from
     * @param streamReader The old Stream reader instance
     */
    virtual ErrCode INTERFACE_FUNC setOldStreamReader(IStreamReader* streamReader) = 0;

    /*!
     * @brief Gets the old Stream reader instance to copy from
     * @param streamReader The old Stream reader instance
     */
    virtual ErrCode INTERFACE_FUNC getOldStreamReader(IStreamReader** streamReader) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the signal to stream reader
     * @param signal The signal which will be handled in stream reader
     */
    virtual ErrCode INTERFACE_FUNC setSignal(ISignal* signal) = 0;

    /*!
     * @brief Gets the signal
     * @param signal The signal which will be handled in stream reader
     */
    virtual ErrCode INTERFACE_FUNC getSignal(ISignal** signal) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the input port to stream reader
     * @param port The input port which will be handled in stream reader
     */
    virtual ErrCode INTERFACE_FUNC setInputPort(IInputPort* port) = 0;

    /*!
     * @brief Gets the input port
     * @param port The input port which will be handled in stream reader
     */
    virtual ErrCode INTERFACE_FUNC getInputPort(IInputPort** port) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the value signal read type
     * @param type The value signal read type
     */
    virtual ErrCode INTERFACE_FUNC setValueReadType(SampleType type) = 0;

    /*!
     * @brief Gets the value signal read type
     * @param[out] type The value signal read type
     */
    virtual ErrCode INTERFACE_FUNC getValueReadType(SampleType* type) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the domain signal read type
     * @param type The domain signal read type
     */
    virtual ErrCode INTERFACE_FUNC setDomainReadType(SampleType type) = 0;

    /*!
     * @brief Gets the domain signal read type
     * @param[out] type The domain signal read type
     */
    virtual ErrCode INTERFACE_FUNC getDomainReadType(SampleType* type) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the read mode (Unscaled, Scaled, RawValue)
     * @param mode The read mode
     */
    virtual ErrCode INTERFACE_FUNC setReadMode(ReadMode mode) = 0;

    /*!
     * @brief Gets the read mode (Unscaled, Scaled, RawValue)
     * @param[out] mode The read mode
     */
    virtual ErrCode INTERFACE_FUNC getReadMode(ReadMode* mode) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the read timeout mode
     * @param mode The timeout mode. 
     * if "Any" returns immediatly if there is available data otherwise time-out is exceeded.
     * if "All" waiting until timeout and returns avaiable data if existing. otherwise time-out is exceeded.
     */
    virtual ErrCode INTERFACE_FUNC setReadTimeoutType(ReadTimeoutType type) = 0;

    /*!
     * @brief Gets the read timeout mode
     * @param mode The timeout mode. 
     * if "Any" returns immediatly if there is available data otherwise time-out is exceeded.
     * if "All" waiting until timeout and returns avaiable data if existing. otherwise time-out is exceeded.
     */
    virtual ErrCode INTERFACE_FUNC getReadTimeoutType(ReadTimeoutType* type) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the skip events
     * @param skipEvents The skip events
     */
    virtual ErrCode INTERFACE_FUNC setSkipEvents(Bool skipEvents) = 0;

    /*!
     * @brief Gets the skip events
     * @param[out] skipEvents The skip events
     */
    virtual ErrCode INTERFACE_FUNC getSkipEvents(Bool* skipEvents) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, StreamReaderBuilder, IStreamReaderBuilder)

/*!@}*/
END_NAMESPACE_OPENDAQ