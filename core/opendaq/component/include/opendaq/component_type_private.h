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
#include "opendaq/module_info.h"
#include <coretypes/serializer.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Private interface to component type. Allows for setting the module information and serializing the type for components.
 */
DECLARE_OPENDAQ_INTERFACE(IComponentTypePrivate, IBaseObject)
{
    // [returnSelf]
    /*!
     * @brief Sets the module information.
     * @param info The module information.
     */
    virtual ErrCode INTERFACE_FUNC setModuleInfo(IModuleInfo* info) = 0;

    /*!
     * @brief Serializes the type without its default configuration, in the form components of this type carry it.
     * @param serializer The serializer to write to.
     * @retval OPENDAQ_ERR_NOTIMPLEMENTED The type sort is not carried by components.
     */
    virtual ErrCode INTERFACE_FUNC serializeWithoutDefaultConfig(ISerializer* serializer) = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
