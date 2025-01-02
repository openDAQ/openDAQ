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
#include <opendaq/component_type.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IComponentType, GenericComponentTypePtr, "<opendaq/component_type_ptr.h>")]
 * [interfaceLibrary(IComponentType, "opendaq")]
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
 * @brief Creates a Server type object, with the id, name, description and optional defaultConfig.
 * @param id The unique type ID of the server.
 * @param name The name of the server type.
 * @param description A short description of the server type.
 * @param defaultConfig The property object, to be cloned and returned, each time user creates default
 * configuration object. This way each instance of the server has its own configuration object.
 */
OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, ServerType,
    IString*, id,
    IString*, name,
    IString*, description,
    IPropertyObject*, defaultConfig
)

/*!@}*/

END_NAMESPACE_OPENDAQ
