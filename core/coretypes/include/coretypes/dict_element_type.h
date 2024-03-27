/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
 * @brief Interface enabling introspection into the expected key and value types of the dictionary.
 */
DECLARE_OPENDAQ_INTERFACE(IDictElementType, IBaseObject)
{
    /*!
     * @brief Returns the interface id of the expected key type.
     * @param[out] id The interface id of the expected key type otherwise returns the id of `IUnknown`.
     */
    virtual ErrCode INTERFACE_FUNC getKeyInterfaceId(IntfID* id) = 0;

    /*!
     * @brief Returns the interface id of the expected value type.
     * @param[out] id The interface id of the expected value type otherwise returns the id of `IUnknown`.
     */
    virtual ErrCode INTERFACE_FUNC getValueInterfaceId(IntfID* id) = 0;
};

END_NAMESPACE_OPENDAQ
