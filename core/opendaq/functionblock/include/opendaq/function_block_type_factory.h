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
#include <opendaq/function_block_type_ptr.h>
#include <coretypes/struct_type_factory.h>
#include <coretypes/simple_type_factory.h>
#include <coreobjects/property_object_factory.h>
#include <opendaq/component_type_builder_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_function_block_type
 * @addtogroup opendaq_function_block_type_factories Factories
 * @{
 */

/*!
 * @brief Creates a FunctionBlockType pointer, with the id, name, description and optional defaultConfig.
 * @param id The unique type ID of the function block.
 * @param name The name of the function block. Eg. FFT.
 * @param description A short description of the function block and its behaviour.
 * @param defaultConfig The property object, to be cloned and returned, each time user creates default
 * configuration object. This way each instance of the function block has its own configuration object.
 */
inline FunctionBlockTypePtr FunctionBlockType(const StringPtr& id,
                                              const StringPtr& name,
                                              const StringPtr& description,
                                              const PropertyObjectPtr& defaultConfig = PropertyObject())
{
    FunctionBlockTypePtr obj(FunctionBlockType_Create(id, name, description, defaultConfig));
    return obj;
}

/*!
 * @brief Creates the Struct type object that defines the Function block type struct.
 */
inline StructTypePtr FunctionBlockTypeStructType()
{
    return StructType("functionBlockType",
                      List<IString>("id", "name", "description"),
                      List<IString>("", "", ""),
                      List<IType>(SimpleType(ctString), SimpleType(ctString), SimpleType(ctString)));
}

/*!@}*/

END_NAMESPACE_OPENDAQ
