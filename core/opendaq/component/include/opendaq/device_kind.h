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
#include <coretypes/common.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup objects_utility
 * @addtogroup objects_component_type Component type
 * @{
 */

/*!
 * @brief The kind of device a Device type creates.
 *
 * External devices can't have streamings attached.
 * Mirrored devices mirror a remote device and can have streamings attached.
 * Root devices can only be set as an instance's root device, via `setRootDevice`.
 * External devices can also be set as root, and Mirrored devices can't.
 * A DeviceKind should be External by default when not set.
 */
enum class DeviceKind : EnumType
{
    External = 0,
    Mirrored,
    Root
};

/*!@}*/

END_NAMESPACE_OPENDAQ
