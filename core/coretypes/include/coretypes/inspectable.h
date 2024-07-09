/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <coretypes/baseobject.h>
#include <coretypes/stringobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup coretypes
 * @addtogroup types_inspectable
 * @{
 */

/*!
 * @brief Provides introspection into the object providing openDAQ interface implementations.
 */
DECLARE_OPENDAQ_INTERFACE(IInspectable, IBaseObject)
{
    /*!
     * @brief Retrieves the Ids of interfaces this object implements.
     * To find out the number of interfaces implemented and the needed size of the @p ids you can first call
     * the function with the @p ids as @c nullptr which will then only return the size over the @p idCount parameter.
     * @param[out] idCount The number of interfaces implemented.
     * @param[out] ids The interface ids represented as an IntfID struct, if @c nullptr it is not populated. 
     */
    virtual ErrCode INTERFACE_FUNC getInterfaceIds(SizeT* idCount, IntfID** ids) = 0;

    /**
     * @brief Gets the fully qualified name of the openDAQ object providing the implementation.
     * @param implementationName The actual implementation class name.
     */
    virtual ErrCode INTERFACE_FUNC getRuntimeClassName(IString** implementationName) = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
