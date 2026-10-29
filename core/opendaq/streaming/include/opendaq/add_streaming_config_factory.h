/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
#include <opendaq/add_streaming_config_ptr.h>
#include <coretypes/listobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_add_streaming_config
 * @addtogroup opendaq_add_streaming_config_factories Factories
 * @{
 */

/*!
 * @brief Creates an add-streaming config listing the given transports. The first one starts out selected.
 * @param transports The names of the transports the streaming connection can be established over.
 */
inline AddStreamingConfigPtr AddStreamingConfig(const ListPtr<IString>& transports = List<IString>())
{
    AddStreamingConfigPtr obj(AddStreamingConfig_Create(transports));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
