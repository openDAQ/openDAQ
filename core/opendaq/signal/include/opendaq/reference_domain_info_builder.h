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
#include <opendaq/reference_domain_info.h>

BEGIN_NAMESPACE_OPENDAQ

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
 * @brief Builder component of Reference Domain Info objects. Contains setter methods that allow for Reference Domain Info
 * parameter configuration, and a `build` method that builds the Reference Domain Info.
 */
DECLARE_OPENDAQ_INTERFACE(IReferenceDomainInfoBuilder, IBaseObject)
{
    /*!
     * @brief Builds and returns a Reference Domain Info object using the currently set values of the Builder.
     * @param[out] referenceDomainInfo The built Reference Domain Info.
     */
    virtual ErrCode INTERFACE_FUNC build(IReferenceDomainInfo** referenceDomainInfo) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the Reference Domain ID.
     * @param referenceDomainId The Reference Domain ID.
     *
     * TODO description
     */
    virtual ErrCode INTERFACE_FUNC setReferenceDomainId(IString* referenceDomainId) = 0;

    /*!
     * @brief Gets the Reference Domain ID.
     * @param[out] referenceDomainId The Reference Domain ID.
     *
     * TODO description
     */
    virtual ErrCode INTERFACE_FUNC getReferenceDomainId(IString** referenceDomainId) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the Reference Domain Offset.
     * @param referenceDomainOffset The Reference Domain Offset.
     *
     * TODO description
     */
    virtual ErrCode INTERFACE_FUNC setReferenceDomainOffset(IInteger* referenceDomainOffset) = 0;

    /*!
     * @brief Gets the Reference Domain Offset.
     * @param[out] referenceDomainOffset The Reference Domain Offset.
     *
     * TODO description
     */
    virtual ErrCode INTERFACE_FUNC getReferenceDomainOffset(IInteger** referenceDomainOffset) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the value that indicates the Reference Time Source.
     * @param referenceTimeSource The value that indicates the Reference Time Source.
     *
     * TODO description
     */
    virtual ErrCode INTERFACE_FUNC setReferenceTimeSource(TimeSource referenceTimeSource) = 0;

    /*!
     * @brief Gets the value that indicates the Reference Time Source.
     * @param[out] referenceTimeSource The value that indicates the Reference Time Source.
     *
     * TODO description
     */
    virtual ErrCode INTERFACE_FUNC getReferenceTimeSource(TimeSource* referenceTimeSource) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the value that indicates if offset is used.
     * @param[out] usesOffset The value that indicates if offset is used.
     *
     * TODO description
     */
    virtual ErrCode INTERFACE_FUNC setUsesOffset(UsesOffset usesOffset) = 0;

    /*!
     * @brief Gets the value that indicates if offset is used.
     * @param[out] usesOffset The value that indicates if offset is used.
     *
     * TODO description
     */
    virtual ErrCode INTERFACE_FUNC getUsesOffset(UsesOffset* usesOffset) = 0;
};
/*!@}*/

/*!
 * @ingroup opendaq_reference_domain_info
 * @addtogroup opendaq_reference_domain_info_factories Factories
 * @{
 */

/*!
 * @brief Reference Domain Info builder factory that creates a builder object with no parameters configured.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, ReferenceDomainInfoBuilder, IReferenceDomainInfoBuilder)

/*!
 * @brief Reference Domain Info copy factory that creates a Reference Domain Info builder object from a
 * different Reference Domain Info, copying its parameters.
 * @param referenceDomainInfoToCopy The Reference Domain Info of which configuration should be copied.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, ReferenceDomainInfoBuilderFromExisting, IReferenceDomainInfoBuilder, IReferenceDomainInfo*, referenceDomainInfoToCopy)

/*!@}*/

END_NAMESPACE_OPENDAQ
