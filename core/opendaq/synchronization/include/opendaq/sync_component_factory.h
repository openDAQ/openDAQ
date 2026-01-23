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
#include <opendaq/sync_component_ptr.h>
#include <opendaq/sync_component2_ptr.h>
#include <opendaq/sync_interface_ptr.h>
#include <opendaq/context_ptr.h>
#include <opendaq/component_ptr.h>
#include <coretypes/string_ptr.h>

BEGIN_NAMESPACE_OPENDAQ
/*!
 * @ingroup opendaq_synchronization_path
 * @addtogroup opendaq_sync_component_factories Factories
 * @{
 */

/*!
 * @brief Creates a synchronization component.
 * @param context The Context. Most often the creating function-block/device passes its own Context to the SyncComponent.
 * @param parent The parent component.
 * @param localId The local ID of the component.
 */
inline SyncComponentPtr SyncComponent(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId)
{
    return { SyncComponent_Create(context, parent, localId) };
}

/*!
 * @brief Creates a synchronization component 2.
 * @param context The Context. Most often the creating function-block/device passes its own Context to the SyncComponent2.
 * @param parent The parent component.
 * @param localId The local ID of the component.
 */
inline SyncComponent2Ptr SyncComponent2(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId)
{
    return { SyncComponent2_Create(context, parent, localId, nullptr, nullptr, true) };
}

/*!@}*/

END_NAMESPACE_OPENDAQ