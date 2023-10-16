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
#include <opendaq/input_port_config_ptr.h>
#include <opendaq/context_ptr.h>
#include <opendaq/input_port_notifications_ptr.h>

BEGIN_NAMESPACE_OPENDAQ
/*!
 * @ingroup opendaq_input_port
 * @addtogroup opendaq_input_port_factories Factories
 * @{
 */

/*!
 * @brief Creates an input port.
 * @param context The Context. Most often the creating function-block passes its own Context to the Folder.
 * @param parent The parent component.
 * @param localId The local ID of the component.
 */
inline InputPortConfigPtr InputPort(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId)
{
    return { InputPort_Create(context, parent, localId) };
}

/*!@}*/

END_NAMESPACE_OPENDAQ
