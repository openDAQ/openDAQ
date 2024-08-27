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
#include <coretypes/baseobject.h>
#include <coretypes/dictobject.h>
#include <coretypes/integer.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IInteger, IntegerPtr, "<coretypes/integer.h>")]
 */

/*!
 * @brief Internal Permissions interface. It should be used only in openDAQ core implementation files.
 */
DECLARE_OPENDAQ_INTERFACE(IPermissionsInternal, IBaseObject)
{
    // [templateType(permissions, IString, IInteger)]
    /*!
     * @brief Returns a dictionary of assigned permissions for each group.
     * @param permissions[out] A dictionary of assigned permissions for each group.
     */
    virtual ErrCode INTERFACE_FUNC getAssigned(IDict** permissions) = 0;
};

END_NAMESPACE_OPENDAQ
