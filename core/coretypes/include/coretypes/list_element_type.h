/*
 * Copyright 2022-2025 openDAQ d.o.o.
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

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Interface enabling introspection into the expected element type of the list.
 */
DECLARE_OPENDAQ_INTERFACE(IListElementType, IBaseObject)
{
    /*!
     * @brief Returns the interface id of the expected list element type.
     * @param[out] id The interface id of the expected element type otherwise returns the id of `IUnknown`.
     */
    virtual ErrCode INTERFACE_FUNC getElementInterfaceId(IntfID* id) = 0;
};

END_NAMESPACE_OPENDAQ
