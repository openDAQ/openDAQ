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
#include <opendaq/input_port_config.h>

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
     * @brief Adds a signal that will be read by the multi reader
     * @param signal The signal that will be read by the multi reader
     */
    virtual ErrCode INTERFACE_FUNC addSignal(ISignal* signal) = 0;

    // [elementType(signals, ISignal), returnSelf]
    /*!
     * @brief Adds signals that will be read by the multi reader
     * @param signals The signals that will be read by the multi reader
     */
    virtual ErrCode INTERFACE_FUNC addSignals(IList* signals) = 0;

    // [returnSelf]
    /*!
     * @brief Adds a port that will be read from by the multi reader
     * @param port The port that will be read by the multi reader
     */
    virtual ErrCode INTERFACE_FUNC addInputPort(IInputPort* port) = 0;

    // [elementType(ports, IInputPort), returnSelf]
    /*!
     * @brief Adds ports that will be read from by the multi reader
     * @param ports The ports that will be read by the multi reader
     */
    virtual ErrCode INTERFACE_FUNC addInputPorts(IList* ports) = 0;

    // [templateType(components, IComponent)]
    /*!
     * @brief Gets the list of read components (signals or ports)
     * @param[out] components The list of read components
     */
    virtual ErrCode INTERFACE_FUNC getSourceComponents(IList** components) = 0;

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
     *
     * NOTE: THIS IS CURRENTLY IGNORED AND IS ALWAYS SET TO ReadTimeoutType::All
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

    // [returnSelf]
    /*!
     * @brief Set maximum distance between signals in fractions of domain unit
     * @param offsetTolerance Ratio that define offset tolerance as a fraction of domain unit.
     */
    virtual ErrCode INTERFACE_FUNC setTickOffsetTolerance(IRatio* offsetTolerance) = 0;

    /*!
     * @brief Get maximum distance between signals in fractions of domain unit
     * @param offsetTolerance[out] Ratio that define offset tolerance as a fraction of domain unit.
     */
    virtual ErrCode INTERFACE_FUNC getTickOffsetTolerance(IRatio** offsetTolerance) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the "AllowDifferentSamplingRates" multi reader parameter.
     * @param allowDifferentRates If set to `false`, the multi reader will only accept signals with the same sampling rate.
     */
    virtual ErrCode INTERFACE_FUNC setAllowDifferentSamplingRates(Bool allowDifferentRates) = 0;

    /*!
     * @brief Gets the "AllowDifferentSamplingRates" multi reader parameter.
     * @param allowDifferentRates If set to `false`, the multi reader will only accept signals with the same sampling rate.
     */
    virtual ErrCode INTERFACE_FUNC getAllowDifferentSamplingRates(Bool* allowDifferentRates) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the notification method of ports created/owned by the multi reader. The default notification method is Unspecified.
     * @param notificationMethod The notification method to be used.
     *
     * If "Unspecified", the reader keeps the mode of the input port. When building with signals, "Unspecified" is an invalid configuration.
     */
    virtual ErrCode INTERFACE_FUNC setInputPortNotificationMethod(PacketReadyNotification notificationMethod) = 0;

    /*!
     * @brief Gets the notification method of ports created/owned by the multi reader. The default notification method is SameThread.
     * @param notificationMethod The notification method to be used.
     *
     * If "Unspecified", the reader keeps the mode of the input port. When building with signals, "Unspecified" is an invalid configuration.
     */
    virtual ErrCode INTERFACE_FUNC getInputPortNotificationMethod(PacketReadyNotification* notificationMethod) = 0;

    // [elementType(notificationMethods, PacketReadyNotification), returnSelf]
    /*!
     * @brief Sets the notification methods of ports created/owned by the multi reader. The default notification method is Unspecified.
     * @param notificationMethods The notification methods to be used.
     *
     * The list of methods corresponds to the list of reader components (signals, input ports). Both the size and order of both must match if configured.
     * If a method is set to "Unspecified", the reader keeps the mode of the input port. When building with signals, "Unspecified" is an invalid configuration.
     */
    virtual ErrCode INTERFACE_FUNC setInputPortNotificationMethods(IList* notificationMethods) = 0;

    // [elementType(notificationMethods, PacketReadyNotification)]
    /*!
     * @brief Gets the notification methods of ports created/owned by the multi reader. The default notification method is Unspecified.
     * @param notificationMethods The notification methods to be used.
     *
     * The list of methods corresponds to the list of reader components (signals, input ports). Both the size and order of both must match if configured.
     * If a method is set to "Unspecified", the reader keeps the mode of the input port. When building with signals, "Unspecified" is an invalid configuration.
     */
    virtual ErrCode INTERFACE_FUNC getInputPortNotificationMethods(IList * *notificationMethods) = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, MultiReaderBuilder, IMultiReaderBuilder)

END_NAMESPACE_OPENDAQ
