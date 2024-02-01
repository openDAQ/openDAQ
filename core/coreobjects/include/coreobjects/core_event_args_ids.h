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
#include <coretypes/common.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_utility
 * @addtogroup opendaq_core_events Core Event IDs
 * @{
 */

namespace core_event_ids
{
    constexpr Int PropertyValueChanged = 0;
    constexpr Int PropertyObjectUpdateEnd = 10;
    constexpr Int PropertyAdded = 20;
    constexpr Int PropertyRemoved = 30;
    constexpr Int ComponentAdded = 40;
    constexpr Int ComponentRemoved = 50;
    constexpr Int SignalConnected = 60;
    constexpr Int SignalDisconnected = 70;
    constexpr Int DataDescriptorChanged = 80;
    constexpr Int ComponentUpdateEnd = 90;
    constexpr Int AttributeChanged = 100;
    constexpr Int TagsChanged = 110;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
