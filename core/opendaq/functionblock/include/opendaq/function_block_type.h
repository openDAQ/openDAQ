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
#include <coreobjects/component_type.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IComponentType, GenericComponentTypePtr, "<coreobjects/component_type_ptr.h>")]
 * [interfaceLibrary(IComponentType, "coreobjects")]
 */

/*!
 * @ingroup opendaq_function_blocks
 * @addtogroup opendaq_function_block_type Function block type
 * @{
 */

/*!
 * @brief Provides information about the function block.
 */
DECLARE_OPENDAQ_INTERFACE(IFunctionBlockType, IComponentType)
{
};
/*!@}*/

/*!
 * @ingroup opendaq_function_block_type
 * @addtogroup opendaq_function_block_type_factories Factories
 * @{
 */

/*!
 * @brief Creates a Function block type object, with the id, name, description and
 * optional createDefaultConfigCallback.
 * @param id The unique type ID of the function block.
 * @param name The name of the function block. Eg. FFT.
 * @param description A short description of the function block and its behaviour.
 * @param createDefaultConfigCallback The callback, which is called, when user want to create new
 * default configuration object.
 * Function needs to create and return property object. On each call, we need to create new object,
 * because we want that each instance of the function block has its own configuration object.
 */
OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, FunctionBlockType,
    IString*, id,
    IString*, name,
    IString*, description,
    IFunction*, createDefaultConfigCallback
)

/*!@}*/

END_NAMESPACE_OPENDAQ
