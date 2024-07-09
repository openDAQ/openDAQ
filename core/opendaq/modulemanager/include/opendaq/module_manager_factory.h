/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <opendaq/module_manager_ptr.h>
#include <opendaq/logger_ptr.h>
#include <opendaq/context_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_module_manager
 * @addtogroup opendaq_module_manager_factories Factories
 * @{
 */

/*!
 * @brief Creates a ModuleManager that loads modules at a given search path. If the search path is empty,
 * it searches the executable folder and its subfolders. Otherwise, it searches the for the relative directory
 * based on the current working directory.
 * @param searchPath The location of the module libraries.
 */
inline ModuleManagerPtr ModuleManager(const StringPtr& searchPath)
{
    ModuleManagerPtr obj(ModuleManager_Create(searchPath));
    return obj;
}

/*!
 * @brief Creates a ModuleManager that loads modules at given search paths. If the search path is empty,
 * it searches the executable folder and its subfolders. Otherwise, it searches the for the relative directory
 * based on the current working directory.
 * @param paths The locations of the module libraries.
 */
inline ModuleManagerPtr ModuleManagerMultiplePaths(const ListPtr<IString>& paths)
{
    ModuleManagerPtr obj(ModuleManagerMultiplePaths_Create(paths));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
