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
#include <opendaq/module_info_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_modules
 * @defgroup opendaq_module_info_factories Factories
 * @{
 */

/*!
 * @brief Creates a Module Info with its `versionInfo`, `name`, and `id`
 * fields configured.
 *
 * @param versionInfo The semantic version information.
 * @param name The module name.
 * @param id The module id.
 * @return The ModuleInfo object.
 */
inline ModuleInfoPtr ModuleInfo(const VersionInfoPtr& versionInfo, const StringPtr& name, const StringPtr& id)
{
    return ModuleInfoPtr{ModuleInfo_Create(versionInfo, name, id)};
}

/*!
 * @brief Creates the Struct type object that defines the Module Info struct.
 */
inline StructTypePtr ModuleInfoStructType()
{
    return StructType("ModuleInfo",
                      List<IString>("VersionInfo", "Name", "Id"),
                      List<IBaseObject>(VersionInfo(0, 0, 0), "", ""),
                      List<IType>(VersionInfoStructType(), SimpleType(ctString), SimpleType(ctString)));
}

/*!@}*/

END_NAMESPACE_OPENDAQ
