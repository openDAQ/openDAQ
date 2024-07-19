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
#include <opendaq/component_type_builder_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup coreobjects_unit
 * @addtogroup coreobjects_unit_factories Factories
 * @{
 */

/*!
 * @brief Creates a ComponentTypeBuilder with the type sort set to "Streaming".
 */
inline ComponentTypeBuilderPtr StreamingTypeBuilder()
{
    ComponentTypeBuilderPtr obj(StreamingTypeBuilder_Create());
    return obj;
}

/*!
 * @brief Creates a ComponentTypeBuilder with the type sort set to "Device".
 */
inline ComponentTypeBuilderPtr DeviceTypeBuilder()
{
    ComponentTypeBuilderPtr obj(DeviceTypeBuilder_Create());
    return obj;
}

/*!
 * @brief Creates a ComponentTypeBuilder with the type sort set to "FunctionBlock".
 */
inline ComponentTypeBuilderPtr FunctionBlockTypeBuilder()
{
    ComponentTypeBuilderPtr obj(FunctionBlockTypeBuilder_Create());
    return obj;
}

/*!
 * @brief Creates a ComponentTypeBuilder with the type sort set to "Server".
 */
inline ComponentTypeBuilderPtr ServerTypeBuilder()
{
    ComponentTypeBuilderPtr obj(ServerTypeBuilder_Create());
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
