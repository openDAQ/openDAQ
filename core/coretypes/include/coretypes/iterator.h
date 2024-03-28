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
#include <cstdint>
#include <coretypes/common.h>
#include <coretypes/baseobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @addtogroup types_containers
 * @{
 */

/*!
 * @brief Use this interface to iterator through items of a container object.
 *
 * Call moveNext function in a loop until it returns OPENDAQ_NO_MORE_ITEMS. Iteration
 * cannot be restarted. In this case a new iterator must be created.
 *
 * Example:
 * @code
 * IIterator* it = ...;
 *
 * while (it->moveNext() != OPENDAQ_NO_MORE_ITEMS)
 * {
 *      IBaseObject* obj;
 *      it->getCurrent(&obj);
 *      // do something with obj
 * }
  @endcode
 */
DECLARE_OPENDAQ_INTERFACE(IIterator, IBaseObject)
{
    /*!
     * @brief Moves iterator to next position.
     */
    virtual ErrCode INTERFACE_FUNC moveNext() = 0;

    /*!
     * @brief Gets the object at current iterator position.
     * @param[out] obj Object at current iterator position.
     * @retval OPENDAQ_NO_MORE_ITEMS Iterator is over the last item position.
     */
    virtual ErrCode INTERFACE_FUNC getCurrent(IBaseObject** obj) const = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
