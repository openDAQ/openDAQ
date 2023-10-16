/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <opendaq/server_type_ptr.h>
#include <coretypes/function_ptr.h>
#include <coretypes/struct_type_factory.h>
#include <coretypes/simple_type_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup structure_server_type
 * @addtogroup structure_server_type_factories Factories
 * @{
 */

/*!
 * @brief Creates a ServerTypeConfigPtr pointer, with the id, name, description and
 * optional createDefaultConfigCallback.
 * @param id The unique type ID of the server.
 * @param name The name of the server type.
 * @param description A short description of the server type.
 * @param createDefaultConfigCallback The callback, which is called, when user want to create new default
 * configuration object.
 * Function needs to create and return property object. On each call, we need to create new object,
 * because we want that each instance of the server has its own configuration object.
 */
inline ServerTypePtr ServerType(const StringPtr& id,
                                      const StringPtr& name,
                                      const StringPtr& description,
                                      const FunctionPtr& createDefaultConfigCallback = nullptr)
{
    ServerTypePtr obj(ServerType_Create(id, name, description, createDefaultConfigCallback));
    return obj;
}

/*!
 * @brief Creates the Struct type object that defines the Server type struct.
 */
inline StructTypePtr ServerTypeStructType()
{
    return StructType("serverType",
                      List<IString>("id", "name", "description"),
                      List<IString>("", "", ""),
                      List<IType>(SimpleType(ctString), SimpleType(ctString), SimpleType(ctString)));
}

/*!@}*/
        
END_NAMESPACE_OPENDAQ
