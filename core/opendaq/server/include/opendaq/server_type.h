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
 * @ingroup structure_servers
 * @addtogroup structure_server_type Server type
 * @{
 */

/*!
 * @brief Provides information about the server.
 */

DECLARE_OPENDAQ_INTERFACE(IServerType, IComponentType)
{
};
/*!@}*/

/*!
 * @ingroup structure_server_type
 * @addtogroup structure_server_type_factories Factories
 * @{
 */

/*!
 * @brief Creates a Server type object, with the id, name, description and
 * optional createDefaultConfigCallback.
 * @param id The unique type ID of the server.
 * @param name The name of the server type.
 * @param description A short description of the server type.
 * @param createDefaultConfigCallback The callback, which is called, when user want to create new default
 * configuration object.
 * Function needs to create and return property object. On each call, we need to create new object,
 * because we want that each instance of the server has its own configuration object.
 */
OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, ServerType,
    IString*, id,
    IString*, name,
    IString*, description,
    IFunction*, createDefaultConfigCallback
)

/*!@}*/

END_NAMESPACE_OPENDAQ
