/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <coretypes/boolean.h>
#include <coretypes/integer.h>
#include <coretypes/stringobject.h>

BEGIN_NAMESPACE_OPENDAQ

enum class TimeSource : EnumType
{
    Unknown,
    Tai,
    Gps,
    Utc
};

enum class UsesOffset : EnumType
{
    Unknown,
    True,
    False
};

struct IReferenceDomainInfoBuilder;

/*#
 * [interfaceSmartPtr(IInteger, IntegerPtr, "<coretypes/integer.h>")]
 * [interfaceSmartPtr(IBoolean, BooleanPtr, "<coretypes/boolean_factory.h>")]
 */

/*!
 * @ingroup opendaq_signals
 * @addtogroup opendaq_reference_domain_info Reference Domain Info
 * @{
 */

/*!
 * @brief If set, gives additional information about the reference domain.
 */
DECLARE_OPENDAQ_INTERFACE(IReferenceDomainInfo, IBaseObject)
{
    /*!
     * @brief Gets the Reference Domain ID.
     * @param[out] referenceDomainId The Reference Domain ID.
     *
     * If set, gives the common identifier of one domain group. 
     * Signals with the same reference Domain ID share a common synchronization source
     * (all the signals in a group either come from the same device 
     * or are synchronized using a protocol, such as PTP, NTP, IRIG, etc.).
     * Those signals can always be read together, implying that a Multi Reader 
     * can be used to read the signals if their sampling rates are compatible.
     */
    virtual ErrCode INTERFACE_FUNC getReferenceDomainId(IString** referenceDomainId) = 0;

    /*!
     * @brief Gets the Reference Domain Offset.
     * @param[out] referenceDomainOffset The Reference Domain Offset.
     *
     * If set, denotes the offset in ticks that must be added to the domain values of the signal 
     * for them to be equal of that of the sync source. The sync source will always have an offset of 0. 
     * This offset is changed only if the sync source changes and should be kept at 0 otherwise, 
     * allowing clients to differentiate between data loss and resync events. 
     * Any device can choose to always keep the offset at 0, representing changes in the offset in 
     * the domain packet values instead. This implementation prevents clients from differentiating 
     * between errors (data loss) and resync events. Additionally, if the offset is not configured, 
     * clients have no way of detecting a resync event in the case of asynchronous signals.
     */
    virtual ErrCode INTERFACE_FUNC getReferenceDomainOffset(IInteger** referenceDomainOffset) = 0;

    /*!
     * @brief Gets the value that indicates the Reference Time Source.
     * @param[out] referenceTimeSource The value that indicates the Reference Time Source.
     *
     * If not set to Unknown, the domain quantity is “time”, and the timestamps are absolute according 
     * to the chosen time standard. The possible values are Gps, Tai, and Utc.
     * This field is used to determine if two signals with different Domain IDs can be read 
     * together. Signals that have configured a Reference Time Source are trusted to have absolute 
     * time stamps that correlate to the chosen time standard (eg. two separate PTP networks, 
     * both driven through GPS can be read together, as their absolute time is the same).
     */
    virtual ErrCode INTERFACE_FUNC getReferenceTimeSource(TimeSource* referenceTimeSource) = 0;

    /*!
     * @brief Gets the value that indicates if offset is used.
     * @param[out] usesOffset The value that indicates if offset is used.
     *
     * If False, a device will contain time jumps due to resync in the domain signal data.
     */
    virtual ErrCode INTERFACE_FUNC getUsesOffset(UsesOffset * usesOffset) = 0;
};
/*!@}*/

/*!
 * @ingroup opendaq_reference_domain_info
 * @addtogroup opendaq_reference_domain_info_factories Factories
 * @{
 */

/*!
 * @brief Creates a Reference Domain Info using Builder
 * @param builder Reference Domain Info Builder
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, ReferenceDomainInfoFromBuilder, IReferenceDomainInfo, IReferenceDomainInfoBuilder*, builder)

/*!@}*/

END_NAMESPACE_OPENDAQ
