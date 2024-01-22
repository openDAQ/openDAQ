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
#include <opendaq/component_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_component
 * @addtogroup opendaq_component_factories Factories
 * @{
 */

/*!
 * @brief Creates a component.
 * @param context The Context. Most often the creating function-block/device passes its own Context to the Folder.
 * @param parent The parent component.
 * @param localId The local ID of the component.
 */
inline ComponentPtr Component(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId)
{
    ComponentPtr obj(Component_Create(context, parent, localId, nullptr));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
