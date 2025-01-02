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
#include <coretypes/baseobject.h>
#include <coretypes/listobject.h>
#include <coretypes/coretype.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup objects_utility
 * @addtogroup objects_callable_info CallableInfo
 * @{
 */

/*!
 * @brief Provides information about the argument count and types, as well as the return type of
 * Function/Procedure-type properties.
 *
 * A callable should be invoked with the parameter types specified in the `arguments` field, in the
 * order listed.
 *
 * A Procedure-type Property will not have a return type configured in its Callable info field.
 * 
 * Argument info objects implement the Struct methods internally and are Core type `ctStruct`.
 */
DECLARE_OPENDAQ_INTERFACE(ICallableInfo, IBaseObject)
{
    /*!
     * @brief Gets the return type of the callable function.
     * @param[out] type The return type of the callable.
     */
    virtual ErrCode INTERFACE_FUNC getReturnType(CoreType* type) = 0;

    // [elementType(argumentInfo, IArgumentInfo)]
    /*!
     * @brief Gets the list of arguments the callable function/procedure expects.
     * @param[out] argumentInfo the list of arguments of type `ArgumentInfo`.
     */
    virtual ErrCode INTERFACE_FUNC getArguments(IList** argumentInfo) = 0;

    /*!
     * @brief A flag indicating if function is marked as const. A const function promises not to modify the state
     * of the device or any other objects under the openDAQ instance.
     * @param[out] constFlag a flag indicating if the function is marked as const or not.
     */
    virtual ErrCode INTERFACE_FUNC isConst(Bool* constFlag) = 0;
};

/*!@}*/

/*!
 * @addtogroup objects_callable_info_factories Factories
 * @{
 */

/*!
 * @brief Creates a CallableInfo object with the specified arguments and return type.
 * @param argumentInfo The list of `ArgumentInfo` type argument information.
 * @param returnType The return type of the described callable object.
 * @param constFlag A flag indicating if the function is marked as const or not. A const function promises not to modify the state
 * of the device or any other objects under the openDAQ instance.
 */
OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, CallableInfo,
    IList*, argumentInfo,
    CoreType, returnType,
    Bool, constFlag
);

/*!@}*/

END_NAMESPACE_OPENDAQ
