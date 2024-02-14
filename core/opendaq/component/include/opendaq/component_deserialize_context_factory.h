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
#include <opendaq/component_deserialize_context_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_component
 * @addtogroup opendaq_component_factories Factories
 * @{
 */

inline ComponentDeserializeContextPtr ComponentDeserializeContext(const ContextPtr& context,
                                                                  const ComponentPtr& root,
                                                                  const ComponentPtr& parent,
                                                                  const StringPtr& localId,
                                                                  IntfID* id = nullptr)
{
    ComponentDeserializeContextPtr obj(ComponentDeserializeContext_Create(context, root, parent, localId, id));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
