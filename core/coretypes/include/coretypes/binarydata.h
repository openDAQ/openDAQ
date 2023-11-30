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
#include <coretypes/baseobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup types_containers
 * @defgroup types_binary_data BinaryData
 * @{
 */

/*!
 * @brief Represents binary large object (BLOB).
 *
 * Binary data is just a continuously allocated memory of a specific size. A client can get a pointer to
 * internal buffer and size it.
 */
DECLARE_OPENDAQ_INTERFACE(IBinaryData, IBaseObject)
{
    /*!
     * @brief Gets the address of the buffer.
     * @param[out] data The buffer's starting address.
     */
    virtual ErrCode INTERFACE_FUNC getAddress(void** data) = 0;
    /*!
     * @brief Gets the size of the buffer.
     * @param[out] size The buffer's size.
     */
    virtual ErrCode INTERFACE_FUNC getSize(SizeT * size) = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, BinaryData, const SizeT, size)

END_NAMESPACE_OPENDAQ
