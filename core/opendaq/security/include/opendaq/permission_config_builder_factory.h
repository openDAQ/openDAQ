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
#include <opendaq/permission_config_builder_ptr.h>
#include <opendaq/permission_config_builder_impl.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_security
 * @addtogroup opendaq_security_permission_config_builder Factories
 * @{
 */

/*!
 * @brief Creates a permission config builder object.
 */
inline PermissionConfigBuilderPtr PermissionConfigBuilder()
{
    PermissionConfigBuilderPtr obj(createWithImplementation<IPermissionConfigBuilder, PermissionConfigBuilderImpl>());
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
