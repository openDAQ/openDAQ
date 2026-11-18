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

#include <coretypes/objectptr.h>
#include <coretypes/cloneable.h>
#include <coretypes/ctutils.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Gets a frozen copy of a container, such as a dictionary or a list.
 * @param container The container to copy; an unassigned one gives an unassigned copy.
 *
 * The copy keeps the order of the items. A cloneable item is cloned, any other item is shared.
 */
template <typename TPtr>
TPtr FrozenClone(const TPtr& container)
{
    if (!container.assigned())
        return TPtr();

    BaseObjectPtr cloned;
    checkErrorInfo(container.template asPtr<ICloneable>()->clone(&cloned));

    TPtr copy = cloned.template asPtr<typename TPtr::DeclaredInterface>();
    copy.freeze();
    return copy;
}

END_NAMESPACE_OPENDAQ
