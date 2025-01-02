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
#include <coretypes/common.h>
#include <coretypes/baseobject.h>
#include <opendaq/multi_reader.h>
#include <opendaq/input_port.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_readers
 * @addtogroup opendaq_multi_reader Multi reader builder
 * @{
 */

/*!
 * @brief Builder component of Multi reader objects. Contains setter methods to configure the Multi reader parameters
 * and a `build` method that builds the Unit object.
 */
DECLARE_OPENDAQ_INTERFACE(IMultiReaderBuilder, IBaseObject)
{
    /*!
     * @brief Builds and returns a Multi reader object using the currently set values of the Builder.
     * @param[out] multiReader The built Multi reader.
     */
    virtual ErrCode INTERFACE_FUNC build(IMultiReader** multiReader) = 0;

    // [returnSelf]
    /*!
     * @brief Adds the signal to list in multi reader
     * @param signal The signal which will be handled in multi reader
     */
    virtual ErrCode INTERFACE_FUNC addSignal(ISignal* signal) = 0;

    // [returnSelf]
    /*!
     * @brief Adds the input port to list in multi reader
     * @param port The input port which will be handled in multi reader
     */
    virtual ErrCode INTERFACE_FUNC addInputPort(IInputPort* port) = 0;

    // [templateType(ports, IComponent)]
    /*!
     * @brief Gets the list of input ports
     * @param[out] ports The list of input ports
     */
    virtual ErrCode INTERFACE_FUNC getSourceComponents(IList** ports) = 0;
   
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
     * @param type The timeout mode. 
     * if "Any" returns immediately if there is available data otherwise time-out is exceeded.
     * if "All" waiting until timeout and returns available data if existing. otherwise time-out is exceeded.
     */
    virtual ErrCode INTERFACE_FUNC setReadTimeoutType(ReadTimeoutType type) = 0;

    /*!
     * @brief Gets the read timeout mode
     * @param type The timeout mode. 
     * if "Any" returns immediately if there is available data otherwise time-out is exceeded.
     * if "All" waiting until timeout and returns available data if existing. otherwise time-out is exceeded.
     */
    virtual ErrCode INTERFACE_FUNC getReadTimeoutType(ReadTimeoutType* type) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the required common sample rate
     * @param sampleRate The required common sample rate
     */
    virtual ErrCode INTERFACE_FUNC setRequiredCommonSampleRate(Int sampleRate) = 0;

    /*!
     * @brief Gets the required common sample rate
     * @param sampleRate The required common sample rate
     */
    virtual ErrCode INTERFACE_FUNC getRequiredCommonSampleRate(Int* sampleRate) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the start on full unit of domain
     * @param enabled enable/disable start on full unit of domain
     */
    virtual ErrCode INTERFACE_FUNC setStartOnFullUnitOfDomain(Bool enabled) = 0;

    /*!
     * @brief Gets the start on full unit of domain
     * @param enabled enable/disable start on full unit of domain
     */
    virtual ErrCode INTERFACE_FUNC getStartOnFullUnitOfDomain(Bool* enabled) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the minimal number of samples to read.
     * @param minReadCount The minimal number of samples to read.
     *
     * If set, the reader will return 0 for `getAvailableCount` if less than `minReadCount`
     * samples are available. It will also never read less than `minReadCount` samples. The
     * default value is 1.
     */
    virtual ErrCode INTERFACE_FUNC setMinReadCount(SizeT minReadCount) = 0;

    /*!
     * @brief Gets the minimal number of samples to read.
     * @param[out] minReadCount The minimal number of samples to read.
     *
     * If set, the reader will return 0 for `getAvailableCount` if less than `minReadCount`
     * samples are available. It will also never read less than `minReadCount` samples. The
     * default value is 1.
     */
    virtual ErrCode INTERFACE_FUNC getMinReadCount(SizeT* minReadCount) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, MultiReaderBuilder, IMultiReaderBuilder)

/*!@}*/
END_NAMESPACE_OPENDAQ
