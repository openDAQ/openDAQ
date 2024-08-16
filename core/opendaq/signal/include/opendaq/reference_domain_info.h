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
 * @brief TODO brief
 *
 * TODO description
 */
DECLARE_OPENDAQ_INTERFACE(IReferenceDomainInfo, IBaseObject)
{
    /*!
     * @brief Gets the Reference Domain ID.
     * @param[out] referenceDomainId The Reference Domain ID.
     *
     * TODO description
     */
    virtual ErrCode INTERFACE_FUNC getReferenceDomainId(IString** referenceDomainId) = 0;

    /*!
     * @brief Gets the Reference Domain Offset.
     * @param[out] referenceDomainOffset The Reference Domain Offset.
     *
     * TODO description
     */
    virtual ErrCode INTERFACE_FUNC getReferenceDomainOffset(IInteger** referenceDomainOffset) = 0;

    /*!
     * @brief Gets the value that indicates the Reference Time Source.
     * @param[out] referenceTimeSource The value that indicates the Reference Time Source.
     *
     * TODO description
     */
    virtual ErrCode INTERFACE_FUNC getReferenceTimeSource(TimeSource* referenceTimeSource) = 0;

    /*!
     * @brief Gets the value that indicates if offset is used.
     * @param[out] usesOffset The value that indicates if offset is used.
     *
     * TODO description
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
