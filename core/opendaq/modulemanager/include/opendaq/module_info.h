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
#include <coretypes/stringobject.h>
#include <coretypes/version_info.h>

BEGIN_NAMESPACE_OPENDAQ
/*#
 * [interfaceLibrary(IPropertyObject, "coreobjects")]
 */

/*!
 * @ingroup opendaq_modules
 * @defgroup opendaq_module_info ModuleInfo
 * @{
 */

/*!
 * @brief Give module information
 * composing of:
 *  - version info (major, minor, patch)
 *  - name
 *  - id.
 */
DECLARE_OPENDAQ_INTERFACE(IModuleInfo, IBaseObject)
{
    /*!
     * @brief Retrieves the module version information.
     * @param[out] version The semantic version information.
     */
    virtual ErrCode INTERFACE_FUNC getVersionInfo(IVersionInfo** version) = 0;

    /*!
     * @brief Gets the module name.
     * @param[out] name The module name.
     */
    virtual ErrCode INTERFACE_FUNC getName(IString** name) = 0;

    /*!
     * @brief Gets the module id.
     * @param[out] id The module id.
     */
    virtual ErrCode INTERFACE_FUNC getId(IString** id) = 0;
};
/*!
 * @}
 */

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, ModuleInfo, IVersionInfo*, versionInfo, IString*, name, IString*, id);

END_NAMESPACE_OPENDAQ
