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
#include <opendaq/reference_domain_info_builder_ptr.h>
#include <opendaq/reference_domain_info_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_reference_domain_info
 * @addtogroup opendaq_reference_domain_info_factories Factories
 * @{
 */

/*!
 * @brief Reference Domain Info builder factory that creates a builder object with no parameters configured.
 */
inline ReferenceDomainInfoBuilderPtr ReferenceDomainInfoBuilder()
{
    ReferenceDomainInfoBuilderPtr obj(ReferenceDomainInfoBuilder_Create());
    return obj;
}

/*!
 * @brief Reference Domain Info copy factory that creates a Reference Domain Info builder object from a
 * different Reference Domain Info, copying its parameters.
 * @param referenceDomainInfo The Reference Domain Info of which configuration should be copied.
 */
inline ReferenceDomainInfoBuilderPtr ReferenceDomainInfoBuilderCopy(const ReferenceDomainInfoPtr& referenceDomainInfo)
{
    ReferenceDomainInfoBuilderPtr obj(ReferenceDomainInfoBuilderFromExisting_Create(referenceDomainInfo));
    return obj;
}

/*!
 * @brief Creates a Reference Domain Info using Builder
 * @param builder Reference Domain Info Builder
 */
inline ReferenceDomainInfoPtr ReferenceDomainInfoFromBuilder(const ReferenceDomainInfoBuilderPtr& builder)
{
    ReferenceDomainInfoPtr obj(ReferenceDomainInfoFromBuilder_Create(builder));
    return obj;
}

/*!
 * @brief Creates the Struct type object that defines the Reference Domain Info struct.
 */
inline StructTypePtr ReferencnceDomainInfoStructType()
{
    return StructType("ReferencnceDomainInfo",
                      List<IString>("ReferenceDomainId", "ReferenceDomainOffset", "ReferenceDomainIsAbsolute"),
                      List<IBaseObject>(nullptr, nullptr, nullptr),
                      List<IType>(SimpleType(ctString), SimpleType(ctInt), SimpleType(ctBool)));
}

/*!@}*/

END_NAMESPACE_OPENDAQ
