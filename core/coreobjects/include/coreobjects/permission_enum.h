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
 * @brief Enumeration of available access permissions
 */
enum class Permission : EnumType
{
    None = 0x0,    // The user has no permissions on the object.
    Read = 0x1,    // The user can see and read an object.
    Write = 0x2,   // The user can change or write to the object.
    Execute = 0x4  // The user can execute an action attached to the object.
};

END_NAMESPACE_OPENDAQ
