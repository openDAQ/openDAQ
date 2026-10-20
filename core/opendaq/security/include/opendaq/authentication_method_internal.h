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
#include <coretypes/baseobject.h>
#include <coretypes/dictobject.h>

BEGIN_NAMESPACE_OPENDAQ

// RTGen is told (via the `INTERNAL` keyword on its `rtgen(...)` CMake call) to generate only its C++ smart pointer - no bindings.

/*!
 * @brief Internal helpers to `IAuthenticationMethod`.
 */
DECLARE_OPENDAQ_INTERFACE(IAuthenticationMethodInternal, IBaseObject)
{
    // [templateType(credentials, IString, IString)]
    /*!
     * @brief Checks whether `credentials` is usable as-is for this method - every field it marks required
     * (`ICredentialField::isRequired`) is present among `credentials`'s keys with a non-empty value. A field
     * that isn't required is never checked, whether present or not.
     * @param credentials A credential - a dictionary of field value(s), keyed by their own field id.
     * @param[out] accepts `True` if `credentials` satisfies every required field.
     * @retval OPENDAQ_ERR_INVALIDPARAMETER if `credentials` is unassigned.
     * @retval OPENDAQ_ERR_NOT_SUPPORTED if this method has no fields at all - there is nothing to check,
     * so callers are expected to never call this for one.
     */
    virtual ErrCode INTERFACE_FUNC acceptsCredentials(IDict* credentials, Bool* accepts) = 0;
};

END_NAMESPACE_OPENDAQ
