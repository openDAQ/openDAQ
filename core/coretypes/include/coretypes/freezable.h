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
 * @addtogroup types_utility
 * @{
 */

/*!
 * @brief Transforms a mutable object to an immutable object.
 *
 * If an object supports IFreezable interface, it is possible to transform it to immutable object.
 * Once the object is frozen, it should not allow to change any of its properties or internal state.
 */

DECLARE_OPENDAQ_INTERFACE(IFreezable, IBaseObject)
{
    /*!
     * @brief Makes the object frozen/immutable.
     * It should return OPENDAQ_IGNORED value if the object is already frozen.
     */
    virtual ErrCode INTERFACE_FUNC freeze() = 0;

    /*!
     * @brief Checks if the objects is frozen/immutable.
     * @param[out] isFrozen The object's frozen/immutable state.
     */
    virtual ErrCode INTERFACE_FUNC isFrozen(Bool* isFrozen) const = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
