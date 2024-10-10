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

#include <coreobjects/callable_info_ptr.h>
#include <utility>
#include <coretypes/struct_type_factory.h>
#include <coretypes/simple_type_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup objects_callable_info
 * @addtogroup objects_callable_info_factories Factories
 * @{
 */

/*!
 * @brief Creates a CallableInfo object that describes a function-type callable with the specified arguments and return type.
 * @param returnType The return type of the described function callable object.
 * @param arguments The list of `IArgumentInfo` type argument information.
 * @param isConst Flag indicating if the function is marked as const. A const function promises not to modify the state
 * of the device or any other objects under the openDAQ instance.
 */
inline CallableInfoPtr FunctionInfo(CoreType returnType, ListPtr<IArgumentInfo> arguments = nullptr, Bool isConst = false)
{
    return CallableInfoPtr::Adopt(CallableInfo_Create(arguments, returnType, isConst));
}

/*!
 * @brief Creates a CallableInfo object that describes a procedure-type callable with the specified arguments.
 * @param arguments The list of `IArgumentInfo` type argument information.
 * @param isConst Flag indicating if the function is marked as const. A const function promises not to modify the state
 * of the device or any other objects under the openDAQ instance.
 */
inline CallableInfoPtr ProcedureInfo(ListPtr<IArgumentInfo> arguments = nullptr, Bool isConst = false)
{
    return FunctionInfo(CoreType::ctUndefined, std::move(arguments), isConst);
}

/*!
 * @brief Creates the Struct type object that defines the Callable info struct.
 */
inline StructTypePtr CallableInfoStructType()
{
    return StructType("CallableInfo",
                      List<IString>("Arguments", "ReturnType", "Const"),
                      List<IBaseObject>(List<IArgumentInfo>(), "", false),
                      List<IType>(SimpleType(ctList), SimpleType(ctInt), SimpleType(ctBool)));
}

/*@}*/

END_NAMESPACE_OPENDAQ
