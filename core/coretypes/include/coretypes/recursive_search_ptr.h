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
#include <coretypes/recursive_search.h>
#include <coretypes/objectptr.h>

BEGIN_NAMESPACE_OPENDAQ

class RecursiveSearchPtr;

template <>
struct InterfaceToSmartPtr<IRecursiveSearch>
{
    typedef RecursiveSearchPtr SmartPtr;
};

/*!
 * @addtogroup types_search
 * @{
 */

class RecursiveSearchPtr : public ObjectPtr<IRecursiveSearch>
{
public:
    using daq::ObjectPtr<IRecursiveSearch>::ObjectPtr;

    RecursiveSearchPtr()
    {
    }

    RecursiveSearchPtr(daq::ObjectPtr<IRecursiveSearch>&& ptr)
        : daq::ObjectPtr<IRecursiveSearch>(std::move(ptr))
    {
    }

    RecursiveSearchPtr(const daq::ObjectPtr<IRecursiveSearch>& ptr)
        : daq::ObjectPtr<IRecursiveSearch>(ptr)
    {
    }

    RecursiveSearchPtr(const RecursiveSearchPtr& other)
        : daq::ObjectPtr<IRecursiveSearch>(other)
    {
    }

    RecursiveSearchPtr(RecursiveSearchPtr&& other) noexcept
        : daq::ObjectPtr<IRecursiveSearch>(std::move(other))
    {
    }

    RecursiveSearchPtr& operator=(const RecursiveSearchPtr& other)
    {
        if (this == &other)
            return *this;

        daq::ObjectPtr<IRecursiveSearch>::operator=(other);
        return *this;
    }

    RecursiveSearchPtr& operator=(RecursiveSearchPtr&& other) noexcept
    {
        if (this == std::addressof(other))
        {
            return *this;
        }

        daq::ObjectPtr<IRecursiveSearch>::operator=(std::move(other));
        return *this;
    }
};

/*!
 * @}
 */

END_NAMESPACE_OPENDAQ
